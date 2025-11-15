#include "ModuleEditor.h"
#include "Application.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <sstream>
#include <cstdarg>
#include "ImGuizmo.h"
#include "ComponentTransform.h"
#include <glm/gtc/type_ptr.hpp>

// Includes para crear GameObjects con geometría
#include "GeometryGenerator.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "ModuleScene.h"
#include "Texture.h"

// Enable experimental GLM extensions used (quaternion utilities)
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>


// Engine console storage definitions
std::vector<std::string> ModuleEditor::engine_log;
std::mutex ModuleEditor::engine_log_mutex;
size_t ModuleEditor::engine_log_max_messages = 8192;
bool ModuleEditor::engine_log_auto_scroll = true;

static GameObject* editor_selected_gameobject = nullptr;

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

    static int geometryCounter = 0;
    std::string objectName = geometryType + "_" + std::to_string(++geometryCounter);

    GameObject* gameObject = app.moduleScene->CreateGameObject(objectName.c_str());

    if (!gameObject) {
        std::cerr << "Error al crear GameObject" << std::endl;
        return;
    }

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

    ComponentMesh* meshComp = static_cast<ComponentMesh*>(
        gameObject->CreateComponent(ComponentType::MESH)
        );

    if (meshComp) {
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

        meshComp->LoadFromGeometry(&geom);
    }

    ComponentMaterial* materialComp = (ComponentMaterial*)gameObject->CreateComponent(ComponentType::MATERIAL);

    ModuleEditor::PushEnginePrintf("GameObject created: %s", objectName.c_str());
}

// ============================================
// IMPLEMENTACIÓN DE LA CLASE ModuleEditor
// ============================================

ModuleEditor::ModuleEditor()
{
    fps_pos = 0;
    fps_count = 0;
    for (int i = 0; i < FPS_HISTORY_SIZE; ++i) fps_history[i] = 0.0f;
}

ModuleEditor::~ModuleEditor()
{
}

bool ModuleEditor::Start()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    auto& app = Application::GetInstance();

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

    ImGui_ImplSDL3_InitForOpenGL(app.window->GetWindow(), app.window->GetContext());
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // ===== Crear framebuffer para el viewport =====
    glGenFramebuffers(1, &sceneFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFramebuffer);

    glGenTextures(1, &sceneTexture);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sceneFBWidth, sceneFBHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTexture, 0);

    glGenRenderbuffers(1, &sceneRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, sceneFBWidth, sceneFBHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, sceneRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        PushEngineLog("ERROR: Scene framebuffer not complete!");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    return true;
}

bool ModuleEditor::Update()
{
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
            bool prev_about = show_about_window;
            bool prev_console = show_console_window;
            bool prev_hier = show_hierarchy_window;
            bool prev_inspector = show_inspector_window;

            ImGui::MenuItem("About", NULL, &show_about_window);
            ImGui::MenuItem("Console", NULL, &show_console_window);
            ImGui::MenuItem("Hierarchy", NULL, &show_hierarchy_window);
            ImGui::MenuItem("Inspector", NULL, &show_inspector_window);

            if (prev_about != show_about_window)
                PushEnginePrintf("About Window %s", show_about_window ? "opened" : "closed");
            if (prev_console != show_console_window)
                PushEnginePrintf("Console Window %s", show_console_window ? "opened" : "closed");
            if (prev_hier != show_hierarchy_window)
                PushEnginePrintf("Hierarchy Window %s", show_hierarchy_window ? "opened" : "closed");
            if (prev_inspector != show_inspector_window)
                PushEnginePrintf("Inspector Window %s", show_inspector_window ? "opened" : "closed");

            ImGui::Separator();

            if (ImGui::BeginMenu("Gizmo"))
            {
                if (ImGui::MenuItem("Translate", "W", currentGizmoOperation == GizmoOperation::TRANSLATE))
                {
                    currentGizmoOperation = GizmoOperation::TRANSLATE;
                    PushEngineLog("Gizmo mode: TRANSLATE");
                }

                if (ImGui::MenuItem("Rotate", "E", currentGizmoOperation == GizmoOperation::ROTATE))
                {
                    currentGizmoOperation = GizmoOperation::ROTATE;
                    PushEngineLog("Gizmo mode: ROTATE");
                }

                if (ImGui::MenuItem("Scale", "R", currentGizmoOperation == GizmoOperation::SCALE))
                {
                    currentGizmoOperation = GizmoOperation::SCALE;
                    PushEngineLog("Gizmo mode: SCALE");
                }

                ImGui::Separator();

                if (ImGui::MenuItem("World Space", NULL, currentGizmoMode == GizmoMode::WORLD))
                {
                    currentGizmoMode = GizmoMode::WORLD;
                    PushEngineLog("Gizmo space: WORLD");
                }

                if (ImGui::MenuItem("Local Space", NULL, currentGizmoMode == GizmoMode::LOCAL))
                {
                    currentGizmoMode = GizmoMode::LOCAL;
                    PushEngineLog("Gizmo space: LOCAL");
                }

                ImGui::Separator();

                ImGui::MenuItem("Use Snap", NULL, &useSnap);

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Configuration"))
            {
                ImGui::MenuItem("Performance", NULL, &show_config_performance);
                ImGui::MenuItem("Modules", NULL, &show_config_modules);
                ImGui::MenuItem("System", NULL, &show_config_system);
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

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

        if (ImGui::BeginMenu("Debug"))
        {
            auto& app = Application::GetInstance();

            if (ImGui::MenuItem("Show AABBs", NULL, app.opengl ? app.opengl->showAABBs : false))
            {
                if (app.opengl)
                {
                    app.opengl->showAABBs = !app.opengl->showAABBs;
                    PushEnginePrintf("AABB visualization: %s", app.opengl->showAABBs ? "ON" : "OFF");
                }
            }

            if (ImGui::MenuItem("Show Grid", NULL, app.opengl->showGrid))
            {
                app.opengl->showGrid = !app.opengl->showGrid;
                PushEnginePrintf("Grid: %s", app.opengl->showGrid ? "ON" : "OFF");
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Update All AABBs"))
            {
                if (app.moduleScene)
                {
                    app.moduleScene->UpdateAllAABBs();
                    PushEngineLog("All AABBs updated");
                }
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

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

    // ===== CALCULAR DIMENSIONES DEL VIEWPORT 3D =====
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();

    float hierWidth = 260.0f;
    float inspWidth = 310.0f;
    float consoleHeight = 200.0f;  // Altura de la consola en la parte inferior

    // El viewport 3D ocupa desde arriba hasta donde empieza la consola
    float viewportHeight = mainViewport->WorkSize.y - consoleHeight;
    float viewportWidth = mainViewport->WorkSize.x;

    // ===== REDIMENSIONAR FRAMEBUFFER SI ES NECESARIO =====
    int newFBWidth = (int)viewportWidth;
    int newFBHeight = (int)viewportHeight;

    if (newFBWidth != sceneFBWidth || newFBHeight != sceneFBHeight)
    {
        sceneFBWidth = std::max(1, newFBWidth);
        sceneFBHeight = std::max(1, newFBHeight);

        // Redimensionar textura
        glBindTexture(GL_TEXTURE_2D, sceneTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sceneFBWidth, sceneFBHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

        // Redimensionar depth/stencil buffer
        glBindRenderbuffer(GL_RENDERBUFFER, sceneRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, sceneFBWidth, sceneFBHeight);
    }

    // ===== RENDERIZAR ESCENA AL FRAMEBUFFER =====
    if (sceneFramebuffer != 0 && Application::GetInstance().moduleScene)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFramebuffer);
        glViewport(0, 0, sceneFBWidth, sceneFBHeight);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Renderizar todos los GameObjects
        Application::GetInstance().moduleScene->RenderScene();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // ===== MOSTRAR TEXTURA DEL FRAMEBUFFER EN IMGUI (Ocupa toda la ventana excepto consola) =====
    // Calcular posición y tamaño del viewport
    ImVec2 viewportWindowPos = ImVec2(mainViewport->WorkPos.x, mainViewport->WorkPos.y);
    ImVec2 viewportWindowSize = ImVec2(viewportWidth, viewportHeight);

    ImGui::SetNextWindowPos(viewportWindowPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(viewportWindowSize, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGuiWindowFlags viewportFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoFocusOnAppearing;

    ImGui::Begin("3D Viewport", nullptr, viewportFlags);

    // Obtener el tamaño real disponible
    ImVec2 contentSize = ImGui::GetContentRegionAvail();

    // Calcular aspect ratio correcto
    float aspect = contentSize.x / contentSize.y;
    ImVec2 imageSize = contentSize;

    // Centrar la imagen si es necesario
    ImVec2 imagePos = ImGui::GetCursorPos();

    ImGui::SetCursorPos(imagePos);
    ImGui::Image((ImTextureID)(intptr_t)sceneTexture, imageSize, ImVec2(0, 1), ImVec2(1, 0));

    // CRÍTICO: Hacer la imagen "clickeable" para mouse picking
    bool imageHovered = ImGui::IsItemHovered();
    bool imageClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

    // CRÍTICO: Actualizar tracking del mouse sobre viewport
    isMouseOverViewport = imageHovered;

    viewportPos = ImGui::GetWindowPos();
    viewportSize = ImGui::GetWindowSize();

    // ===== DIBUJAR GIZMO DENTRO DEL VIEWPORT =====
    HandleGizmo();

    // ===== MOUSE PICKING (dentro del viewport) =====
    if (imageClicked && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver())
    {
        auto& app = Application::GetInstance();

        if (app.camera && app.moduleScene)
        {
            ImVec2 mousePos = ImGui::GetMousePos();
            float relativeX = mousePos.x - viewportPos.x;
            float relativeY = mousePos.y - viewportPos.y;

            PushEnginePrintf("Mouse picking - Relative pos: (%.1f, %.1f) Viewport size: (%.1f, %.1f)",
                relativeX, relativeY, viewportSize.x, viewportSize.y);

            if (relativeX >= 0 && relativeX < viewportSize.x &&
                relativeY >= 0 && relativeY < viewportSize.y)
            {
                Ray pickRay = app.camera->ScreenPointToRay(
                    relativeX,
                    relativeY,
                    (int)viewportSize.x,
                    (int)viewportSize.y
                );

                PushEnginePrintf("Ray created - Origin: (%.2f, %.2f, %.2f) Dir: (%.2f, %.2f, %.2f)",
                    pickRay.origin.x, pickRay.origin.y, pickRay.origin.z,
                    pickRay.direction.x, pickRay.direction.y, pickRay.direction.z);

                app.moduleScene->UpdateAllAABBs();
                GameObject* pickedObject = app.moduleScene->PerformRaycast(pickRay);

                if (pickedObject)
                {
                    app.moduleScene->SetSelectedGameObject(pickedObject);
                    PushEnginePrintf("Picked GameObject: %s", pickedObject->GetName());
                }
                else
                {
                    app.moduleScene->SetSelectedGameObject(nullptr);
                    PushEngineLog("No object picked - selection cleared");
                }
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();

    // ===== MOUSE PICKING (ya no se necesita aquí, se hace dentro del viewport) =====
    // HandleMousePicking();

    // Hierarchy window
    if (show_hierarchy_window)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 hierPos = ImVec2(viewport->WorkPos.x + 10.0f, viewport->WorkPos.y + 10.0f);
        ImGui::SetNextWindowPos(hierPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(250, 400), ImGuiCond_FirstUseEver);
        ImGuiWindowFlags hierFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

        ImGui::Begin("Hierarchy", NULL, hierFlags);

        auto& app = Application::GetInstance();
        if (app.moduleScene)
        {
            const std::vector<GameObject*>& all = app.moduleScene->GetAllGameObjects();

            for (GameObject* go : all)
            {
                if (!go) continue;

                ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf;
                if (go == editor_selected_gameobject || go == app.moduleScene->GetSelectedGameObject())
                {
                    node_flags |= ImGuiTreeNodeFlags_Selected;
                }

                bool node_open = ImGui::TreeNodeEx((void*)go, node_flags, "%s", go->GetName());

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

    // Inspector window
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

            // ===== CONTROLES DE GIZMO =====
            if (ImGui::CollapsingHeader("Gizmo Controls", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Operation Mode:");

                if (ImGui::RadioButton("Translate (W)", currentGizmoOperation == GizmoOperation::TRANSLATE))
                {
                    currentGizmoOperation = GizmoOperation::TRANSLATE;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Rotate (E)", currentGizmoOperation == GizmoOperation::ROTATE))
                {
                    currentGizmoOperation = GizmoOperation::ROTATE;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Scale (R)", currentGizmoOperation == GizmoOperation::SCALE))
                {
                    currentGizmoOperation = GizmoOperation::SCALE;
                }

                ImGui::Separator();

                ImGui::Text("Coordinate Space:");
                if (ImGui::RadioButton("World", currentGizmoMode == GizmoMode::WORLD))
                {
                    currentGizmoMode = GizmoMode::WORLD;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Local", currentGizmoMode == GizmoMode::LOCAL))
                {
                    currentGizmoMode = GizmoMode::LOCAL;
                }

                ImGui::Separator();

                ImGui::Checkbox("Use Snap", &useSnap);

                if (useSnap)
                {
                    ImGui::DragFloat("Translate Snap", &snapValues[0], 0.1f, 0.01f, 10.0f);
                    ImGui::DragFloat("Rotate Snap (deg)", &snapValues[1], 1.0f, 1.0f, 90.0f);
                    ImGui::DragFloat("Scale Snap", &snapValues[2], 0.01f, 0.01f, 1.0f);
                }
            }

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
                            if (inspectorCheckerTex == 0)
                            {
                                inspectorCheckerTex = Texture::CreateCheckerboardTexture(512, 512, 32);
                            }

                            mat->SetOverrideTexture(inspectorCheckerTex, false);
                            inspectorOverrideTarget = (void*)selected;
                        }
                        else
                        {
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
        // Calcular posición de la consola (parte inferior de la ventana)
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float consoleHeight = 200.0f;
        ImVec2 consolePos = ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - consoleHeight);
        ImVec2 consoleSize = ImVec2(viewport->WorkSize.x, consoleHeight);

        ImGui::SetNextWindowPos(consolePos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(consoleSize, ImGuiCond_Always);

        ImGuiWindowFlags consoleFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

        ImGui::Begin("Console", &show_console_window, consoleFlags);

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

    // Config: Performance
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

    // Config: Modules
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
        ImGui::Indent();
        ImGui::TextWrapped("The Renderer module handles drawing the scene using OpenGL.\nIt controls rendering modes (wireframe/fill), clear color, culling and depth testing. Use the Scene/Renderer configuration or debug options in the main UI to toggle wireframe or other renderer-specific debug views.");
        ImGui::Unindent();

        ImGui::Separator();

        ImGui::Text("Input");
        ImGui::Indent();
        ImGui::TextWrapped("The Input module processes keyboard, mouse and gamepad events and exposes settings such as mouse sensitivity. Changes here affect how user input is interpreted by camera and gameplay modules. To change bindings or advanced input behavior edit the Input module or add an input mapping UI in the future.");
        ImGui::Unindent();

        ImGui::Separator();

        ImGui::Text("Textures");
        ImGui::Indent();
        ImGui::TextWrapped("The Textures module is responsible for texture loading and sampling.\nFiltering (Nearest/Linear), mipmap generation and GPU upload behavior are controlled by the renderer/resource manager. Use the material/texture inspector to preview textures and change sampler settings where available.");
        ImGui::Unindent();

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
    // IMPORTANTE: Solo renderizar ImGui, NO la escena 3D
    // La escena ya se renderizó en Update() al framebuffer

    ImGui::Render();

    // Limpiar el backbuffer principal (para ImGui)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, settings.window_width, settings.window_height);
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Configurar para renderizado 2D (ImGui)
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Restaurar estado para próximo frame
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    return true;
}

bool ModuleEditor::CleanUp()
{
    // Limpiar framebuffer
    if (sceneFramebuffer) glDeleteFramebuffers(1, &sceneFramebuffer);
    if (sceneTexture) glDeleteTextures(1, &sceneTexture);
    if (sceneRBO) glDeleteRenderbuffers(1, &sceneRBO);
    if (inspectorCheckerTex) glDeleteTextures(1, &inspectorCheckerTex);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    return true;
}

void ModuleEditor::ProcessEvent(const SDL_Event& event)
{
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
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

            const std::vector<std::string> tex_ext = { "png","jpg","jpeg","bmp","tga","dds","tif","tiff","psd" };
            const std::vector<std::string> model_ext = { "fbx","obj","gltf","glb","dae","3ds" };

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

    ImGui_ImplSDL3_ProcessEvent(&event);
}

void ModuleEditor::HandleGizmo()
{
    auto& app = Application::GetInstance();

    if (!app.camera || !app.moduleScene)
        return;

    GameObject* selected = app.moduleScene->GetSelectedGameObject();
    if (!selected)
        return;

    ComponentTransform* transform = selected->GetComponent<ComponentTransform>();
    if (!transform)
        return;

    ImGuiIO& io = ImGui::GetIO();

    // Solo permitir cambio de modo si no estamos usando el gizmo
    if (!ImGuizmo::IsUsing())
    {
        if (ImGui::IsKeyPressed(ImGuiKey_W))
        {
            currentGizmoOperation = GizmoOperation::TRANSLATE;
            PushEngineLog("Gizmo mode: TRANSLATE");
        }
        if (ImGui::IsKeyPressed(ImGuiKey_E))
        {
            currentGizmoOperation = GizmoOperation::ROTATE;
            PushEngineLog("Gizmo mode: ROTATE");
        }
        if (ImGui::IsKeyPressed(ImGuiKey_R))
        {
            currentGizmoOperation = GizmoOperation::SCALE;
            PushEngineLog("Gizmo mode: SCALE");
        }
    }

    // CRÍTICO: Configurar ImGuizmo correctamente
    ImGuizmo::SetOrthographic(false);

    // Usar la drawlist de la ventana del viewport para que se dibuje dentro
    ImGuizmo::SetDrawlist();

    // Establecer el rect donde se dibujará el gizmo
    ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

    // Habilitar ImGuizmo para que capture el input
    ImGuizmo::Enable(true);

    glm::mat4 view = app.camera->getViewMatrix();
    glm::mat4 projection = app.camera->getProjectionMatrix();

    glm::mat4 modelMatrix = transform->GetGlobalMatrix();

    ImGuizmo::OPERATION operation;
    switch (currentGizmoOperation)
    {
    case GizmoOperation::TRANSLATE: operation = ImGuizmo::TRANSLATE; break;
    case GizmoOperation::ROTATE:    operation = ImGuizmo::ROTATE;    break;
    case GizmoOperation::SCALE:     operation = ImGuizmo::SCALE;     break;
    }

    ImGuizmo::MODE mode = (currentGizmoMode == GizmoMode::LOCAL)
        ? ImGuizmo::LOCAL
        : ImGuizmo::WORLD;

    float* snap = nullptr;
    if (useSnap)
    {
        switch (currentGizmoOperation)
        {
        case GizmoOperation::TRANSLATE: snap = &snapValues[0]; break;
        case GizmoOperation::ROTATE:    snap = &snapValues[1]; break;
        case GizmoOperation::SCALE:     snap = &snapValues[2]; break;
        }
    }

    glm::mat4 deltaMatrix;

    bool manipulated = ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(projection),
        operation,
        mode,
        glm::value_ptr(modelMatrix),
        glm::value_ptr(deltaMatrix),
        snap
    );

    if (manipulated && ImGuizmo::IsUsing())
    {
        glm::vec3 position, scale, skew;
        glm::vec4 perspective;
        glm::quat rotation;

        glm::decompose(modelMatrix, scale, rotation, position, skew, perspective);

        if (!selected->GetParent())
        {
            transform->SetPosition(position);
            transform->SetRotation(rotation);
            transform->SetScale(scale);
        }
        else
        {
            GameObject* parent = selected->GetParent();
            ComponentTransform* parentTransform = parent->GetComponent<ComponentTransform>();

            if (parentTransform)
            {
                glm::mat4 parentMatrix = parentTransform->GetGlobalMatrix();
                glm::mat4 localMatrix = glm::inverse(parentMatrix) * modelMatrix;

                glm::vec3 localPos, localScale, localSkew;
                glm::vec4 localPerspective;
                glm::quat localRotation;

                glm::decompose(localMatrix, localScale, localRotation, localPos, localSkew, localPerspective);

                transform->SetPosition(localPos);
                transform->SetRotation(localRotation);
                transform->SetScale(localScale);
            }
        }

        app.moduleScene->UpdateAllAABBs();
    }
}

void ModuleEditor::HandleMousePicking()
{
    // Esta función ya no se usa - el mouse picking se maneja directamente
    // dentro de la ventana del viewport usando ImGui::IsItemClicked()
}