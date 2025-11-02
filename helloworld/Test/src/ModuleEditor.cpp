#include "ModuleEditor.h"
#include "Application.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <sstream>

// Includes para crear GameObjects con geometría
#include "GeometryGenerator.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"

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
        settings.wireframe = false; // No wireframe state exposed yet
    }
    if (app.camera)
    {
        settings.mouse_sensitivity = 1.0f; // camera has its own sensitivity internally; expose link later
    }

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(app.window->GetWindow(), app.window->GetContext());
    ImGui_ImplOpenGL3_Init("#version 330 core");

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
                // Signal application to quit via SDL event
                SDL_Event evt;
                SDL_zero(evt);
                evt.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&evt);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Demo Window", NULL, &show_demo_window);
            ImGui::MenuItem("Test Window", NULL, &show_test_window);
            ImGui::MenuItem("About", NULL, &show_about_window);

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

        // ============================================
        // MENÚ GEOMETRY - MODIFICADO PARA CREAR GAMEOBJECTS
        // ============================================
        if (ImGui::BeginMenu("Geometry"))
        {
            if (ImGui::MenuItem("Cube"))
            {
                requested_geometry = "Cube";
                CreateGeometryGameObject("Cube");
            }
            if (ImGui::MenuItem("Sphere"))
            {
                requested_geometry = "Sphere";
                CreateGeometryGameObject("Sphere");
            }
            if (ImGui::MenuItem("Cylinder"))
            {
                requested_geometry = "Cylinder";
                CreateGeometryGameObject("Cylinder");
            }
            if (ImGui::MenuItem("Pyramid"))
            {
                requested_geometry = "Pyramid";
                CreateGeometryGameObject("Pyramid");
            }
            if (ImGui::MenuItem("Plane"))
            {
                requested_geometry = "Plane";
                CreateGeometryGameObject("Plane");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Documentation on GitHub"))
            {
                SDL_OpenURL("https://github.com/AsiGamer29/WizardEngine/blob/main/helloworld/docs/Documentation.md");
            }
            if (ImGui::MenuItem("Report a bug"))
            {
                SDL_OpenURL("https://github.com/AsiGamer29/WizardEngine/issues");
            }
            if (ImGui::MenuItem("Download latest"))
            {
                SDL_OpenURL("https://github.com/AsiGamer29/WizardEngine/releases");
            }
            if (ImGui::MenuItem("About"))
            {
                show_about_window = !show_about_window;
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
        }

        ImGui::Checkbox("Show Demo Window", &show_demo_window);

        // Show last requested geometry (for debugging / future connection)
        if (!requested_geometry.empty())
        {
            ImGui::Separator();
            ImGui::Text("Last requested geometry: %s", requested_geometry.c_str());
        }

        ImGui::End();
    }

    // Config: Performance (FPS graph)
    if (show_config_performance)
    {
        ImGui::Begin("Performance", &show_config_performance);
        // prepare data order for plot (ImGui expects 0..count-1)
        int count = fps_count;
        int offset = (fps_pos >= count) ? fps_pos - count : (fps_pos + FPS_HISTORY_SIZE - count);
        // If buffer wrapped, present contiguous data by constructing a temporary array
        if (offset + count <= FPS_HISTORY_SIZE)
        {
            ImGui::PlotLines("FPS", fps_history + offset, count, 0, NULL, 0.0f, 240.0f, ImVec2(0, 80));
        }
        else
        {
            // create temporary array
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
            // apply to real window
            auto& app = Application::GetInstance();
            if (app.window) app.window->SetWindowSize(settings.window_width, settings.window_height);
        }

        bool oldVsync = settings.vsync;
        ImGui::Checkbox("VSync", &settings.vsync);
        if (oldVsync != settings.vsync)
        {
            auto& app = Application::GetInstance();
            if (app.window) app.window->SetVSync(settings.vsync);
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
        }
        ImGui::ColorEdit3("Clear Color", settings.clear_color);
        // Note: apply clear color could be used by OpenGL module; here just store

        ImGui::Separator();
        ImGui::Text("Input");
        float oldSens = settings.mouse_sensitivity;
        ImGui::SliderFloat("Mouse Sensitivity", &settings.mouse_sensitivity, 0.1f, 5.0f);
        if (oldSens != settings.mouse_sensitivity)
        {
            auto& app = Application::GetInstance();
            if (app.camera)
            {
                // Camera currently stores sensitivity privately; for now we can hack by updating camera's private field if it were public.
                // As a safe approach, we can expose camera sensitivity setter later. For now store value for eventual use.
            }
        }
        ImGui::Separator();
        ImGui::Text("Textures");
        ImGui::Combo("Filter", &settings.texture_filter, "Nearest\0Linear\0");
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
    ImGui_ImplSDL3_ProcessEvent(&event);
}