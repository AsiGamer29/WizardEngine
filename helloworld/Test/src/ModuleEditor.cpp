#include "ModuleEditor.h"
#include "Application.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <sstream>
#include <cstdarg>

// Includes para crear GameObjects con geometría
#include "GeometryGenerator.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "ModuleScene.h"
#include "Texture.h"

// Enable experimental GLM extensions used (quaternion utilities)
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/quaternion.hpp>

// Engine console storage definitions
std::vector<std::string> ModuleEditor::engine_log;
std::mutex ModuleEditor::engine_log_mutex;
size_t ModuleEditor::engine_log_max_messages = 8192;
bool ModuleEditor::engine_log_auto_scroll = true;

static GameObject* editor_selected_gameobject = nullptr; // preparación para selección desde editor

void ModuleEditor::PushEngineLog(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(engine_log_mutex);
    engine_log.push_back(msg);
    if (engine_log.size() > engine_log_max_messages)
        engine_log.erase(engine_log.begin(), engine_log.begin() + (engine_log.size() - engine_log_max_messages));
}

void ModuleEditor::PushEnginePrintf(const char* fmt, ...)
{
    char buffer[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    PushEngineLog(std::string(buffer));
}

// ============================================
// FUNCIÓN HELPER PARA CREAR GAMEOBJECTS CON GEOMETRÍA
// ============================================
static void CreateGeometryGameObject(const std::string& geometryType) {
    auto& app = Application::GetInstance();

    if (!app.moduleScene) {
        std::cerr << "ModuleScene no está inicializado" << std::endl;
        return;
    }

    // Crear un nombre único para el GameObject
    static int geometryCounter = 0;
    std::string objectName = geometryType + "_" + std::to_string(++geometryCounter);

    // Crear el GameObject usando ModuleScene
    GameObject* gameObject = app.moduleScene->CreateGameObject(objectName.c_str());

    if (!gameObject) {
        std::cerr << "Error al crear GameObject" << std::endl;
        return;
    }

    // COMPONENTE TRANSFORM (normalmente GameObject ya lo crea por defecto)
    ComponentTransform* transform = static_cast<ComponentTransform*>(
        gameObject->GetComponent(ComponentType::TRANSFORM)
        );

    if (!transform) {
        transform = static_cast<ComponentTransform*>(
            gameObject->CreateComponent(ComponentType::TRANSFORM)
            );
    }

    if (transform) {
        transform->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        transform->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
        transform->SetRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    }

    // COMPONENTE MESH
    ComponentMesh* meshComp = static_cast<ComponentMesh*>(
        gameObject->CreateComponent(ComponentType::MESH)
        );

    if (meshComp) {
        // Crear la geometría según el tipo
        MeshGeometry geom;

        if (geometryType == "Cube") {
            geom = GeometryGenerator::CreateCube(2.0f);
        }
        else if (geometryType == "Sphere") {
            geom = GeometryGenerator::CreateSphere(1.0f, 32, 16);
        }
        else if (geometryType == "Cylinder") {
            geom = GeometryGenerator::CreateCylinder(1.0f, 2.0f, 32);
        }
        else if (geometryType == "Pyramid") {
            geom = GeometryGenerator::CreatePyramid(2.0f, 2.0f);
        }
        else if (geometryType == "Plane") {
            geom = GeometryGenerator::CreatePlane(5.0f, 5.0f);
        }

        // Cargar la geometría en el componente mesh
        meshComp->LoadFromGeometry(&geom);
    }

    // COMPONENTE MATERIAL
    ComponentMaterial* materialComp = (ComponentMaterial*)gameObject->CreateComponent(ComponentType::MATERIAL);

    if (materialComp) {
        // Ya tiene textura checkerboard por defecto
    }

    std::cout << "GameObject creado: " << objectName << std::endl;
    ModuleEditor::PushEnginePrintf("GameObject created: %s", objectName.c_str());
}

// ============================================
// IMPLEMENTACIÓN DE LA CLASE ModuleEditor
// ============================================

ModuleEditor::ModuleEditor()
{
    // initialize fps history
    fps_pos = 0;
    fps_count = 0;
    for (int i = 0; i < FPS_HISTORY_SIZE; ++i) fps_history[i] = 0.0f;
}

ModuleEditor::~ModuleEditor()
{
}

bool ModuleEditor::Start()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Get window and context from Application
    auto& app = Application::GetInstance();

    // Initialize settings from actual modules
    if (app.window)
    {
        settings.window_width = app.window->GetWidth();
        settings.window_height = app.window->GetHeight();
    }
    if (app.opengl)
    {
        settings.wireframe = false;
    }
    if (app.camera)
    {
        settings.mouse_sensitivity = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(app.window->GetWindow(), app.window->GetContext());
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // Log initial status
    PushEngineLog("Starting Engine...");
    PushEnginePrintf("IMGUI: Initialized (version: %s)", ImGui::GetVersion());

    if (app.window)
    {
        PushEnginePrintf("Window: %dx%d", settings.window_width, settings.window_height);
        PushEnginePrintf("VSync: %s", settings.vsync ? "On" : "Off");
    }

    const char* glver = (const char*)glGetString(GL_VERSION);
    PushEnginePrintf("OpenGL Version: %s", glver ? glver : "Unknown");

    PushEnginePrintf("SDL Platform: %s", SDL_GetPlatform());

    PushEngineLog("Engine started.");

    return true;
}

bool ModuleEditor::PreUpdate()
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    return true;
}

bool ModuleEditor::Update()
{
    // record current FPS
    float current_fps = ImGui::GetIO().Framerate;
    fps_history[fps_pos] = current_fps;
    fps_pos = (fps_pos + 1) % FPS_HISTORY_SIZE;
    fps_count = std::min(fps_count + 1, FPS_HISTORY_SIZE);

    // Main menu bar
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
            {
                SDL_Event evt;
                SDL_zero(evt);
                evt.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&evt);
                PushEngineLog("User requested Exit.");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            bool prev_demo = show_demo_window;
            bool prev_test = show_test_window;
            bool prev_about = show_about_window;
            bool prev_console = show_console_window;
            bool prev_hier = show_hierarchy_window;
            bool prev_inspector = show_inspector_window;

            ImGui::MenuItem("Demo Window", NULL, &show_demo_window);
            ImGui::MenuItem("Test Window", NULL, &show_test_window);
            ImGui::MenuItem("About", NULL, &show_about_window);
            ImGui::MenuItem("Console", NULL, &show_console_window);
            ImGui::MenuItem("Hierarchy", NULL, &show_hierarchy_window);
            ImGui::MenuItem("Inspector", NULL, &show_inspector_window);

            if (prev_demo != show_demo_window)
                PushEnginePrintf("Demo Window %s", show_demo_window ? "opened" : "closed");
            if (prev_test != show_test_window)
                PushEnginePrintf("Test Window %s", show_test_window ? "opened" : "closed");
            if (prev_about != show_about_window)
                PushEnginePrintf("About Window %s", show_about_window ? "opened" : "closed");
            if (prev_console != show_console_window)
                PushEnginePrintf("Console Window %s", show_console_window ? "opened" : "closed");
            if (prev_hier != show_hierarchy_window)
                PushEnginePrintf("Hierarchy Window %s", show_hierarchy_window ? "opened" : "closed");
            if (prev_inspector != show_inspector_window)
                PushEnginePrintf("Inspector Window %s", show_inspector_window ? "opened" : "closed");

            // Configuration submenu inside View
            if (ImGui::BeginMenu("Configuration"))
            {
                ImGui::MenuItem("Performance", NULL, &show_config_performance);
                ImGui::MenuItem("Modules", NULL, &show_config_modules);
                ImGui::MenuItem("System", NULL, &show_config_system);
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        // MENÚ GEOMETRY - CREA GAMEOBJECTS
        if (ImGui::BeginMenu("Geometry"))
        {
            if (ImGui::MenuItem("Cube"))
            {
                requested_geometry = "Cube";
                CreateGeometryGameObject("Cube");
                PushEnginePrintf("Created geometry: %s", "Cube");
            }
            if (ImGui::MenuItem("Sphere"))
            {
                requested_geometry = "Sphere";
                CreateGeometryGameObject("Sphere");
                PushEnginePrintf("Created geometry: %s", "Sphere");
            }
            if (ImGui::MenuItem("Cylinder"))
            {
                requested_geometry = "Cylinder";
                CreateGeometryGameObject("Cylinder");
                PushEnginePrintf("Created geometry: %s", "Cylinder");
            }
            if (ImGui::MenuItem("Pyramid"))
            {
                requested_geometry = "Pyramid";
                CreateGeometryGameObject("Pyramid");
                PushEnginePrintf("Created geometry: %s", "Pyramid");
            }
            if (ImGui::MenuItem("Plane"))
            {
                requested_geometry = "Plane";
                CreateGeometryGameObject("Plane");
                PushEnginePrintf("Created geometry: %s", "Plane");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Documentation on GitHub"))
            {
                SDL_OpenURL("https://github.com/AsiGamer29/WizardEngine/blob/main/helloworld/docs/Documentation.md");
                PushEngineLog("Opened Documentation URL.");
            }
            if (ImGui::MenuItem("Report a bug"))
            {
                SDL_OpenURL("https://github.com/AsiGamer29/WizardEngine/issues");
                PushEngineLog("Opened Issue Tracker URL.");
            }
            if (ImGui::MenuItem("Download latest"))
            {
                SDL_OpenURL("https://github.com/AsiGamer29/WizardEngine/releases");
                PushEngineLog("Opened Releases URL.");
            }
            if (ImGui::MenuItem("About"))
            {
                show_about_window = !show_about_window;
                PushEnginePrintf("Toggled About: %s", show_about_window ? "open" : "closed");
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // Show demo window
    if (show_demo_window)
    {
        ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
        ImGui::ShowDemoWindow(&show_demo_window);
    }

    // Show custom test window
    if (show_test_window)
    {
        ImGui::SetNextWindowPos(ImVec2(650, 50), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);

        ImGui::Begin("Test Window", &show_test_window);

        ImGui::Text("=== IMGUI IS WORKING! ===");
        ImGui::Separator();
        ImGui::Text("Hello from ImGui!");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
            1000.0f / ImGui::GetIO().Framerate,
            ImGui::GetIO().Framerate);

        ImGui::Separator();

        if (ImGui::Button("Toggle Demo Window"))
        {
            show_demo_window = !show_demo_window;
            PushEnginePrintf("Demo Window %s", show_demo_window ? "opened" : "closed");
        }

        ImGui::Checkbox("Show Demo Window", &show_demo_window);

        if (!requested_geometry.empty())
        {
            ImGui::Separator();
            ImGui::Text("Last requested geometry: %s", requested_geometry.c_str());
        }

        ImGui::End();
    }

    // Hierarchy window
    if (show_hierarchy_window)
    {
        
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 hierPos = ImVec2(viewport->WorkPos.x + 10.0f, viewport->WorkPos.y + 10.0f);
        ImGui::SetNextWindowPos(hierPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(250, 400), ImGuiCond_FirstUseEver); 
        ImGuiWindowFlags hierFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse; 

        ImGui::Begin("Hierarchy", NULL, hierFlags);

        // Obtener GameObjects desde ModuleScene
        auto& app = Application::GetInstance();
        if (app.moduleScene)
        {
            const std::vector<GameObject*>& all = app.moduleScene->GetAllGameObjects();

            // Mostrar en lista plana por ahora
            for (GameObject* go : all)
            {
                if (!go) continue;

                ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf;
                // marcar seleccionado
                if (go == editor_selected_gameobject || go == app.moduleScene->GetSelectedGameObject())
                {
                    node_flags |= ImGuiTreeNodeFlags_Selected;
                }

                // Usamos TreeNodeEx para poder mostrar icono y seleccionar
                bool node_open = ImGui::TreeNodeEx((void*)go, node_flags, "%s", go->GetName());

                // Detectar click para seleccionar
                if (ImGui::IsItemClicked())
                {
                    editor_selected_gameobject = go; 
                    app.moduleScene->SetSelectedGameObject(go);
                    PushEnginePrintf("Selected GameObject: %s", go->GetName());
                }

                if (node_open)
                {
                    ImGui::TreePop();
                }
            }
        }
        else
        {
            ImGui::Text("ModuleScene not available");
        }

        ImGui::End();
    }

    // Inspector window (nuevo)
    if (show_inspector_window)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float inspectorW = 300.0f;
        float inspectorH = 500.0f;
        ImVec2 inspPos = ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - inspectorW - 10.0f, viewport->WorkPos.y + 10.0f);
        ImGui::SetNextWindowPos(inspPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(inspectorW, inspectorH), ImGuiCond_FirstUseEver); 
        ImGuiWindowFlags inspFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse; 

        ImGui::Begin("Inspector", &show_inspector_window, inspFlags);

        auto& app = Application::GetInstance();
        GameObject* selected = nullptr;
        if (app.moduleScene)
            selected = app.moduleScene->GetSelectedGameObject();

        
        if ((void*)selected != inspectorOverrideTarget && inspectorOverrideTarget != nullptr)
        {
            
            GameObject* prev = (GameObject*)inspectorOverrideTarget;
            if (prev)
            {
                ComponentMaterial* prevMat = prev->GetComponent<ComponentMaterial>();
                if (prevMat)
                {
                    prevMat->ClearOverrideTexture();
                }
            }
            inspectorOverrideTarget = nullptr;
            inspector_show_checkerboard = false;
        }

        if (!selected)
        {
            ImGui::TextDisabled("No GameObject selected");
            ImGui::End();
        }
        else
        {
            ImGui::Text("Selected: %s", selected->GetName());
            ImGui::Separator();

            // --- Transform Section ---
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ComponentTransform* tr = selected->GetComponent<ComponentTransform>();
                if (tr)
                {
                    glm::vec3 pos = tr->GetPosition();
                    glm::vec3 scl = tr->GetScale();
                    glm::quat rotQ = tr->GetRotation();
                    glm::vec3 euler = glm::degrees(glm::eulerAngles(rotQ));

                    float posArr[3] = { pos.x, pos.y, pos.z };
                    if (ImGui::InputFloat3("Position", posArr))
                    {
                        tr->SetPosition(glm::vec3(posArr[0], posArr[1], posArr[2]));
                    }

                    float rotArr[3] = { euler.x, euler.y, euler.z };
                    if (ImGui::InputFloat3("Rotation", rotArr))
                    {
                        // Convert back to quaternion
                        glm::vec3 rads = glm::radians(glm::vec3(rotArr[0], rotArr[1], rotArr[2]));
                        glm::quat newQ = glm::quat(rads);
                        tr->SetRotation(newQ);
                    }

                    float sclArr[3] = { scl.x, scl.y, scl.z };
                    if (ImGui::InputFloat3("Scale", sclArr))
                    {
                        tr->SetScale(glm::vec3(sclArr[0], sclArr[1], sclArr[2]));
                    }
                }
                else
                {
                    ImGui::TextDisabled("No Transform component.");
                }
            }

            // --- Mesh Section ---
            static bool show_normals = false;
            if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ComponentMesh* mesh = selected->GetComponent<ComponentMesh>();
                if (mesh)
                {
                    ImGui::Text("Vertices: %d", (int)mesh->GetVertexCount());
                    ImGui::Text("Indices: %d", (int)mesh->GetIndexCount());
                    ImGui::Text("Triangles: %d", (int)mesh->GetIndexCount() / 3);

                    ImGui::Checkbox("Show Normals", &show_normals);

                    // TODO: Integrar visualización de normales en renderer; por ahora guardar flag en ModuleScene
                    if (app.moduleScene)
                    {
                        app.moduleScene->SetDebugShowNormals(show_normals);
                    }
                }
                else
                {
                    ImGui::TextDisabled("No Mesh component.");
                }
            }

            // --- Texture Section ---
            if (ImGui::CollapsingHeader("Texture", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ComponentMaterial* mat = selected->GetComponent<ComponentMaterial>();
                if (mat)
                {
                    const char* path = mat->GetTexturePath();
                    int w = mat->GetWidth();
                    int h = mat->GetHeight();

                    ImGui::Text("Path: %s", path ? path : "(none)");
                    ImGui::Text("Size: %dx%d", w, h);

                   
                    bool old = inspector_show_checkerboard;
                    ImGui::Checkbox("Use default checkerboard in scene", &inspector_show_checkerboard);

                    if (inspector_show_checkerboard != old)
                    {
                        if (inspector_show_checkerboard)
                        {
                            // enable override on selected material
                            if (inspectorCheckerTex == 0)
                            {
                                inspectorCheckerTex = Texture::CreateCheckerboardTexture(512, 512, 32);
                            }

                            mat->SetOverrideTexture(inspectorCheckerTex, false);
                            inspectorOverrideTarget = (void*)selected;
                        }
                        else
                        {
                            // disable override
                            mat->ClearOverrideTexture();
                            inspectorOverrideTarget = nullptr;
                        }
                    }

                    
                    GLuint previewTex = mat->GetTextureID();
                    ImGui::Image((ImTextureID)(intptr_t)previewTex, ImVec2(128, 128));
                }
                else
                {
                    ImGui::TextDisabled("No Material component.");
                }
            }

            ImGui::End();
        }
    }

    // Console window
    if (show_console_window)
    {
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Console", &show_console_window);

        ImGui::Checkbox("Auto-scroll", &engine_log_auto_scroll);

        ImGui::Separator();

        ImGui::BeginChild("ConsoleRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        {
            std::lock_guard<std::mutex> lock(engine_log_mutex);
            for (const auto& line : engine_log)
            {
                ImGui::TextUnformatted(line.c_str());
            }
            if (engine_log_auto_scroll)
                ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

        ImGui::End();
    }

    // Config: Performance (FPS graph)
    if (show_config_performance)
    {
        ImGui::Begin("Performance", &show_config_performance);
        int count = fps_count;
        int offset = (fps_pos >= count) ? fps_pos - count : (fps_pos + FPS_HISTORY_SIZE - count);
        if (offset + count <= FPS_HISTORY_SIZE)
        {
            ImGui::PlotLines("FPS", fps_history + offset, count, 0, NULL, 0.0f, 240.0f, ImVec2(0, 80));
        }
        else
        {
            static float temp[FPS_HISTORY_SIZE];
            for (int i = 0; i < count; ++i)
                temp[i] = fps_history[(offset + i) % FPS_HISTORY_SIZE];
            ImGui::PlotLines("FPS", temp, count, 0, NULL, 0.0f, 240.0f, ImVec2(0, 80));
        }
        ImGui::Text("Current: %.1f FPS", fps_history[(fps_pos + FPS_HISTORY_SIZE - 1) % FPS_HISTORY_SIZE]);
        ImGui::End();
    }

    // Config: Modules (settings)
    if (show_config_modules)
    {
        ImGui::Begin("Modules Configuration", &show_config_modules);
        ImGui::Text("Window");
        int oldW = settings.window_width;
        int oldH = settings.window_height;
        ImGui::SliderInt("Width", &settings.window_width, 640, 3840);
        ImGui::SliderInt("Height", &settings.window_height, 480, 2160);
        if (oldW != settings.window_width || oldH != settings.window_height)
        {
            auto& app = Application::GetInstance();
            if (app.window) app.window->SetWindowSize(settings.window_width, settings.window_height);
            PushEnginePrintf("Window resized to %dx%d", settings.window_width, settings.window_height);
        }

        bool oldVsync = settings.vsync;
        ImGui::Checkbox("VSync", &settings.vsync);
        if (oldVsync != settings.vsync)
        {
            auto& app = Application::GetInstance();
            if (app.window) app.window->SetVSync(settings.vsync);
            PushEnginePrintf("VSync %s", settings.vsync ? "enabled" : "disabled");
        }

        ImGui::Separator();
        ImGui::Text("Renderer");
        bool oldWire = settings.wireframe;
        ImGui::Checkbox("Wireframe", &settings.wireframe);
        if (oldWire != settings.wireframe)
        {
            auto& app = Application::GetInstance();
            if (app.opengl)
            {
                if (settings.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
            PushEnginePrintf("Wireframe %s", settings.wireframe ? "enabled" : "disabled");
        }
        ImGui::ColorEdit3("Clear Color", settings.clear_color);

        ImGui::Separator();
        ImGui::Text("Input");
        float oldSens = settings.mouse_sensitivity;
        ImGui::SliderFloat("Mouse Sensitivity", &settings.mouse_sensitivity, 0.1f, 5.0f);
        if (oldSens != settings.mouse_sensitivity)
        {
            PushEnginePrintf("Mouse sensitivity changed to %.2f", settings.mouse_sensitivity);
        }
        
        ImGui::Separator();
        ImGui::Text("Textures");
        int oldFilter = settings.texture_filter;
        ImGui::Combo("Filter", &settings.texture_filter, "Nearest\0Linear\0");
        if (oldFilter != settings.texture_filter)
        {
            PushEnginePrintf("Texture filter set to %s", settings.texture_filter == 0 ? "Nearest" : "Linear");
        }
        ImGui::End();
    }

    // Config: System info
    if (show_config_system)
    {
        ImGui::Begin("System Info", &show_config_system);
        ImGui::Text("Platform: %s", SDL_GetPlatform());

        int sdl_ver = SDL_GetVersion();
        int sdl_major = sdl_ver / 1000000;
        int sdl_minor = (sdl_ver / 1000) % 1000;
        int sdl_patch = sdl_ver % 1000;
        ImGui::Text("SDL Version: %d.%d.%d", sdl_major, sdl_minor, sdl_patch);

        ImGui::Text("CPU Count: %d", SDL_GetNumLogicalCPUCores());
        ImGui::Text("CPU Cache Line Size: %d bytes", SDL_GetCPUCacheLineSize());
        ImGui::Text("System RAM (MB): %d", SDL_GetSystemRAM());

        const char* glver = (const char*)glGetString(GL_VERSION);
        ImGui::Text("OpenGL Version: %s", glver ? glver : "Unknown");

        ImGui::Text("DevIL: not detected (placeholder)");
        ImGui::End();
    }

    // About window
    if (show_about_window)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("About WizardEngine", &show_about_window))
        {
            ImGui::Text("WizardEngine v0.1");
            ImGui::Separator();
            ImGui::Text("Team:");
            ImGui::BulletText("Asier");
            ImGui::BulletText("Aniol");
            ImGui::BulletText("Sauc");
            ImGui::Separator();
            ImGui::Text("Libraries:");
            ImGui::BulletText("SDL3");
            ImGui::BulletText("Dear ImGui");
            ImGui::BulletText("GLM");
            ImGui::Separator();
            ImGui::TextWrapped("MIT License\n\nCopyright (c) WizardEngine\n\nPermission is hereby granted, free of charge, to any person obtaining a copy of this software...");

            ImGui::End();
        }
    }

    return true;
}

bool ModuleEditor::PostUpdate()
{
    // Rendering
    ImGui::Render();

    // Configura para renderizado 2D
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Restaura el estado
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    return true;
}

bool ModuleEditor::CleanUp()
{
    // Shutdown
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    return true;
}

void ModuleEditor::ProcessEvent(const SDL_Event& event)
{
    // Detect file drop events and log filename only
    if (event.type == SDL_EVENT_DROP_FILE)
    {
        const char* data = event.drop.data;
        if (data)
        {
            std::string path(data);
            std::string name;
            size_t pos = path.find_last_of("/\\");
            if (pos != std::string::npos && pos + 1 < path.size())
                name = path.substr(pos + 1);
            else
                name = path;

            std::string ext;
            size_t dot = name.find_last_of('.');
            if (dot != std::string::npos && dot + 1 < name.size())
                ext = name.substr(dot + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

            const std::vector<std::string> tex_ext = {"png","jpg","jpeg","bmp","tga","dds","tif","tiff","psd"};
            const std::vector<std::string> model_ext = {"fbx","obj","gltf","glb","dae","3ds"};

            if (std::find(tex_ext.begin(), tex_ext.end(), ext) != tex_ext.end())
            {
                PushEnginePrintf("Texture dropped: %s", name.c_str());
            }
            else if (std::find(model_ext.begin(), model_ext.end(), ext) != model_ext.end())
            {
                PushEnginePrintf("Model dropped: %s", name.c_str());
            }
            else
            {
                PushEnginePrintf("File dropped: %s", name.c_str());
            }
        }
    }

    // Forward event to ImGui backend
    ImGui_ImplSDL3_ProcessEvent(&event);
}