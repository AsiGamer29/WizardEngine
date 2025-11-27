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
        std::cerr << "ModuleScene no esta inicializado" << std::endl;
        return;
    }

    static int geometryCounter = 0;
    std::string objectName = geometryType + "_" + std::to_string(++geometryCounter);

    // CreateGameObject ya crea el Transform automaticamente
    GameObject* gameObject = app.moduleScene->CreateGameObject(objectName.c_str(), app.moduleScene->GetRoot());

    if (!gameObject) {
        std::cerr << "Error al crear GameObject" << std::endl;
        return;
    }

    // El Transform ya existe, solo lo obtenemos (NO lo creamos de nuevo)
    ComponentTransform* transform = gameObject->GetComponent<ComponentTransform>();

    if (!transform) {
        ModuleEditor::PushEngineLog("CRITICAL ERROR: GameObject created without Transform!");
        return;
    }

    // Ya tiene valores por defecto, pero los podemos ajustar si queremos
    transform->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    transform->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
    transform->SetRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

    // CREAR COMPONENTE MESH
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
        ModuleEditor::PushEnginePrintf("Mesh loaded for: %s", objectName.c_str());
    }
    else {
        ModuleEditor::PushEngineLog("ERROR: Failed to create Mesh component");
    }

    // CREAR COMPONENTE MATERIAL
    ComponentMaterial* materialComp = static_cast<ComponentMaterial*>(
        gameObject->CreateComponent(ComponentType::MATERIAL)
        );

    if (materialComp) {
        GLuint checkerTex = Texture::CreateCheckerboardTexture(512, 512, 32);
        materialComp->SetTexture(checkerTex, "checkerboard_default", 3);
        ModuleEditor::PushEnginePrintf("GameObject created: %s (with checkerboard texture)", objectName.c_str());
    }
    else {
        ModuleEditor::PushEnginePrintf("GameObject created: %s (WARNING: no material)", objectName.c_str());
    }

    // Actualizar AABBs
    app.moduleScene->UpdateAllAABBs();
}

// ============================================
// FUNCIÓN HELPER PARA DIBUJAR NODOS RECURSIVAMENTE
// ============================================
static void DrawGameObjectNode(GameObject* go, Application& app)
{
    if (!go) return;

    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (go == app.moduleScene->GetSelectedGameObject())
    {
        node_flags |= ImGuiTreeNodeFlags_Selected;
    }

    const std::vector<GameObject*>& children = go->GetChildren();
    if (children.empty())
    {
        node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    bool node_open = ImGui::TreeNodeEx((void*)go, node_flags, "%s", go->GetName());

    if (ImGui::IsItemClicked())
    {
        app.moduleScene->SetSelectedGameObject(go);
        ModuleEditor::PushEnginePrintf("Selected GameObject: %s", go->GetName());
    }

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        ImGui::SetDragDropPayload("HIERARCHY_NODE", &go, sizeof(GameObject*));
        ImGui::Text("Move: %s", go->GetName());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_NODE"))
        {
            GameObject* draggedGO = *(GameObject**)payload->Data;

            bool isValid = true;
            GameObject* checkParent = go;
            while (checkParent)
            {
                if (checkParent == draggedGO)
                {
                    isValid = false;
                    break;
                }
                checkParent = checkParent->GetParent();
            }

            if (isValid && draggedGO != go)
            {
                draggedGO->SetParent(go);
                ModuleEditor::PushEnginePrintf("%s is now child of %s",
                    draggedGO->GetName(), go->GetName());
                app.moduleScene->UpdateAllAABBs();
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (node_open)
    {
        if (!children.empty())
        {
            for (GameObject* child : children)
            {
                DrawGameObjectNode(child, app);
            }
        }

        if (!(node_flags & ImGuiTreeNodeFlags_Leaf))
        {
            ImGui::TreePop();
        }
    }
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

static void CreateEmptyGameObject() {
    auto& app = Application::GetInstance();

    if (!app.moduleScene) {
        std::cerr << "ModuleScene no esta inicializado" << std::endl;
        return;
    }

    static int emptyCounter = 0;
    std::string objectName = "Empty_" + std::to_string(++emptyCounter);

    // CreateGameObject ya crea el Transform automaticamente
    GameObject* gameObject = app.moduleScene->CreateGameObject(objectName.c_str(), app.moduleScene->GetRoot());

    if (!gameObject) {
        std::cerr << "Error al crear GameObject" << std::endl;
        return;
    }

    // El Transform ya existe, solo lo obtenemos
    ComponentTransform* transform = gameObject->GetComponent<ComponentTransform>();

    if (!transform) {
        ModuleEditor::PushEngineLog("CRITICAL ERROR: GameObject created without Transform!");
        return;
    }

    // Valores por defecto (position 0, scale 1, rotation identity)
    transform->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    transform->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
    transform->SetRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

    ModuleEditor::PushEnginePrintf("Empty GameObject created: %s", objectName.c_str());

    // Actualizar AABBs
    app.moduleScene->UpdateAllAABBs();
}

bool ModuleEditor::PreUpdate()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

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

        if (ImGui::BeginMenu("GameObject"))
        {
            if (ImGui::MenuItem("Create Empty"))
            {
                CreateEmptyGameObject();
                PushEngineLog("Created empty GameObject");
            }

            ImGui::Separator();

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

    // ===== CALCULAR DIMENSIONES ESTILO UNITY =====
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();

    float hierWidth = 260.0f;      // Ancho de Hierarchy (izquierda)
    float inspWidth = 320.0f;      // Ancho de Inspector (derecha)
    float consoleHeight = 200.0f;  // Altura de la consola (abajo)

    // El viewport 3D está en el centro, entre hierarchy e inspector
    float viewportWidth = mainViewport->WorkSize.x - hierWidth - inspWidth;
    float viewportHeight = mainViewport->WorkSize.y - consoleHeight;

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

    // ===== MOSTRAR TEXTURA DEL FRAMEBUFFER EN IMGUI (Centro, entre Hierarchy e Inspector) =====
    ImVec2 viewportWindowPos = ImVec2(mainViewport->WorkPos.x + hierWidth, mainViewport->WorkPos.y);
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

    // CRÍTICO: Actualizar tracking del viewport PRIMERO
    viewportPos = ImGui::GetWindowPos();
    viewportSize = ImGui::GetContentRegionAvail();

    // Obtener posición donde dibujar
    ImVec2 imagePos = ImGui::GetCursorScreenPos();

    // Dibujar la textura de la escena usando DrawList (no captura input)
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddImage(
        (ImTextureID)(intptr_t)sceneTexture,
        imagePos,
        ImVec2(imagePos.x + viewportSize.x, imagePos.y + viewportSize.y),
        ImVec2(0, 1),
        ImVec2(1, 0)
    );

    // Avanzar el cursor para que ImGui sepa que usamos este espacio
    ImGui::Dummy(viewportSize);

    // Tracking del mouse
    bool imageHovered = ImGui::IsItemHovered();
    bool imageClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    isMouseOverViewport = imageHovered;

    // ===== DIBUJAR GIZMO DENTRO DEL VIEWPORT =====
    // CRÍTICO: El gizmo se dibuja DESPUÉS de la imagen pero ANTES del mouse picking
    HandleGizmo();

    // ===== MOUSE PICKING (dentro del viewport) =====
    // SOLO hacer picking si NO estamos usando el gizmo
    if (imageClicked && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver())
    {
        auto& app = Application::GetInstance();

        if (app.camera && app.moduleScene)
        {
            ImVec2 mousePos = ImGui::GetMousePos();
            float relativeX = mousePos.x - viewportPos.x;
            float relativeY = mousePos.y - viewportPos.y;

            if (relativeX >= 0 && relativeX < viewportSize.x &&
                relativeY >= 0 && relativeY < viewportSize.y)
            {
                Ray pickRay = app.camera->ScreenPointToRay(
                    relativeX,
                    relativeY,
                    (int)viewportSize.x,
                    (int)viewportSize.y
                );

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

    // Hierarchy window - OCUPA TODA LA ALTURA IZQUIERDA CON JERARQUÍA RECURSIVA
    if (show_hierarchy_window)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 hierPos = ImVec2(viewport->WorkPos.x, viewport->WorkPos.y);
        ImVec2 hierSize = ImVec2(hierWidth, viewport->WorkSize.y);

        ImGui::SetNextWindowPos(hierPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(hierSize, ImGuiCond_Always);
        ImGuiWindowFlags hierFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

        ImGui::Begin("Hierarchy", NULL, hierFlags);

        auto& app = Application::GetInstance();
        if (app.moduleScene)
        {
            const std::vector<GameObject*>& all = app.moduleScene->GetAllGameObjects();

            // Dibujar solo los objetos raíz (sin padre)
            for (GameObject* go : all)
            {
                if (!go) continue;

                // Solo dibujar si NO tiene padre (es raíz)
                if (go->GetParent() == nullptr)
                {
                    DrawGameObjectNode(go, app);
                }
            }
        }
        else
        {
            ImGui::Text("ModuleScene not available");
        }

        ImGui::End();
    }

    // Inspector window - OCUPA TODA LA ALTURA DERECHA
    if (show_inspector_window)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 inspPos = ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - inspWidth, viewport->WorkPos.y);
        ImVec2 inspSize = ImVec2(inspWidth, viewport->WorkSize.y);

        ImGui::SetNextWindowPos(inspPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(inspSize, ImGuiCond_Always);
        ImGuiWindowFlags inspFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

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

            // --- Material Section ---
            if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ComponentMaterial* mat = selected->GetComponent<ComponentMaterial>();
                if (mat)
                {
                    const char* path = mat->GetTexturePath();
                    int w = mat->GetWidth();
                    int h = mat->GetHeight();

                    ImGui::Text("Path: %s", path ? path : "(none)");
                    ImGui::Text("Size: %dx%d", w, h);

                    ImGui::Separator();

                    // === ALPHA MODE ===
                    const char* alphaModeNames[] = { "Opaque", "Alpha Test", "Alpha Blend" };
                    int currentMode = (int)mat->GetAlphaMode();
                    if (ImGui::Combo("Alpha Mode", &currentMode, alphaModeNames, 3))
                    {
                        mat->SetAlphaMode((AlphaMode)currentMode);
                        PushEnginePrintf("Alpha mode changed to: %s", alphaModeNames[currentMode]);
                    }

                    // === ALPHA BLEND CONTROLS ===
                    if (mat->GetAlphaMode() == AlphaMode::ALPHA_BLEND)
                    {
                        const char* blendModeNames[] = {
                            "Standard",
                            "Additive",
                            "Multiply",
                            "Screen",
                            "Premultiplied"
                        };
                        int currentBlend = (int)mat->GetBlendMode();
                        if (ImGui::Combo("Blend Mode", &currentBlend, blendModeNames, 5))
                        {
                            mat->SetBlendMode((BlendMode)currentBlend);
                            PushEnginePrintf("Blend mode changed to: %s", blendModeNames[currentBlend]);
                        }

                        // Info sobre cada blend mode
                        ImGui::Spacing();
                        ImGui::TextWrapped("Blended objects are rendered back-to-front automatically.");

                        ImGui::Spacing();
                        ImGui::Text("Blend Mode Info:");
                        switch (mat->GetBlendMode())
                        {
                        case BlendMode::STANDARD:
                            ImGui::BulletText("Standard transparency");
                            ImGui::BulletText("Formula: SrcAlpha + (1-SrcAlpha)*Dst");
                            break;
                        case BlendMode::ADDITIVE:
                            ImGui::BulletText("Additive blending (glow effect)");
                            ImGui::BulletText("Formula: SrcAlpha*Src + Dst");
                            break;
                        case BlendMode::MULTIPLY:
                            ImGui::BulletText("Multiply blending (darken)");
                            ImGui::BulletText("Formula: Dst * Src");
                            break;
                        case BlendMode::SCREEN:
                            ImGui::BulletText("Screen blending (lighten)");
                            ImGui::BulletText("Formula: 1 - (1-Src)*(1-Dst)");
                            break;
                        case BlendMode::PREMULTIPLIED:
                            ImGui::BulletText("Premultiplied alpha");
                            ImGui::BulletText("Formula: Src + (1-SrcAlpha)*Dst");
                            break;
                        }
                    }

                    ImGui::Separator();

                    // === TEXTURE PREVIEW ===
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
                    ImGui::Text("Texture Preview:");
                    ImGui::Image((ImTextureID)(intptr_t)previewTex, ImVec2(128, 128));
                }
                else
                {
                    ImGui::TextDisabled("No Material component.");
                }
            }
        }

        ImGui::End();
    }

    // Console window - PARTE INFERIOR, ENTRE HIERARCHY E INSPECTOR
    if (show_console_window)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        // La consola empieza después de hierarchy y termina antes de inspector
        ImVec2 consolePos = ImVec2(viewport->WorkPos.x + hierWidth,
            viewport->WorkPos.y + viewport->WorkSize.y - consoleHeight);
        ImVec2 consoleSize = ImVec2(viewportWidth, consoleHeight);

        ImGui::SetNextWindowPos(consolePos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(consoleSize, ImGuiCond_Always);

        ImGuiWindowFlags consoleFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

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
    {
        return;
    }

    GameObject* selected = app.moduleScene->GetSelectedGameObject();
    if (!selected)
    {
        return;
    }

    ComponentTransform* transform = selected->GetComponent<ComponentTransform>();
    if (!transform)
    {
        PushEnginePrintf("ERROR: GameObject '%s' has NO Transform!", selected->GetName());
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    // Cambio de modo solo si NO estamos usando el gizmo
    if (!ImGuizmo::IsUsing())
    {
        if (ImGui::IsKeyPressed(ImGuiKey_W))
        {
            currentGizmoOperation = GizmoOperation::TRANSLATE;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_E))
        {
            currentGizmoOperation = GizmoOperation::ROTATE;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_R))
        {
            currentGizmoOperation = GizmoOperation::SCALE;
        }
    }

    // Configurar ImGuizmo
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);
    ImGuizmo::Enable(true);

    glm::mat4 view = app.camera->getViewMatrix();
    glm::mat4 projection = app.camera->getProjectionMatrix();

    // Obtener matriz global del transform
    glm::mat4 modelMatrix = transform->GetGlobalMatrix();

    // Determinar operacion
    ImGuizmo::OPERATION operation;
    switch (currentGizmoOperation)
    {
    case GizmoOperation::TRANSLATE: operation = ImGuizmo::TRANSLATE; break;
    case GizmoOperation::ROTATE:    operation = ImGuizmo::ROTATE;    break;
    case GizmoOperation::SCALE:     operation = ImGuizmo::SCALE;     break;
    }

    // Determinar modo (Local/World)
    ImGuizmo::MODE mode = (currentGizmoMode == GizmoMode::LOCAL)
        ? ImGuizmo::LOCAL
        : ImGuizmo::WORLD;

    // Snap
    float* snap = nullptr;
    float snapArray[3] = { 0.0f, 0.0f, 0.0f };
    if (useSnap)
    {
        switch (currentGizmoOperation)
        {
        case GizmoOperation::TRANSLATE:
            snapArray[0] = snapArray[1] = snapArray[2] = snapValues[0];
            snap = snapArray;
            break;
        case GizmoOperation::ROTATE:
            snapArray[0] = snapArray[1] = snapArray[2] = snapValues[1];
            snap = snapArray;
            break;
        case GizmoOperation::SCALE:
            snapArray[0] = snapArray[1] = snapArray[2] = snapValues[2];
            snap = snapArray;
            break;
        }
    }

    // DIBUJAR Y MANIPULAR GIZMO
    bool manipulated = ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(projection),
        operation,
        mode,
        glm::value_ptr(modelMatrix),
        nullptr,
        snap
    );

    // SI SE MANIPULO, APLICAR CAMBIOS
    if (manipulated)
    {
        // Descomponer la nueva matriz global
        glm::vec3 newPos, newScale, skew;
        glm::vec4 perspective;
        glm::quat newRot;

        glm::decompose(modelMatrix, newScale, newRot, newPos, skew, perspective);

        // Si tiene padre, convertir a coordenadas locales
        if (selected->GetParent())
        {
            GameObject* parent = selected->GetParent();
            ComponentTransform* parentTransform = parent->GetComponent<ComponentTransform>();

            if (parentTransform)
            {
                glm::mat4 parentGlobalMatrix = parentTransform->GetGlobalMatrix();
                glm::mat4 localMatrix = glm::inverse(parentGlobalMatrix) * modelMatrix;

                glm::vec3 localPos, localScale, localSkew;
                glm::vec4 localPerspective;
                glm::quat localRot;

                glm::decompose(localMatrix, localScale, localRot, localPos, localSkew, localPerspective);

                // Aplicar segun operacion
                switch (currentGizmoOperation)
                {
                case GizmoOperation::TRANSLATE:
                    transform->SetPosition(localPos);
                    break;
                case GizmoOperation::ROTATE:
                    transform->SetRotation(localRot);
                    break;
                case GizmoOperation::SCALE:
                    transform->SetScale(localScale);
                    break;
                }
            }
        }
        else
        {
            // Sin padre, usar valores globales directamente
            switch (currentGizmoOperation)
            {
            case GizmoOperation::TRANSLATE:
                transform->SetPosition(newPos);
                break;
            case GizmoOperation::ROTATE:
                transform->SetRotation(newRot);
                break;
            case GizmoOperation::SCALE:
                transform->SetScale(newScale);
                break;
            }
        }

        // Actualizar AABBs
        app.moduleScene->UpdateAllAABBs();
    }
}
