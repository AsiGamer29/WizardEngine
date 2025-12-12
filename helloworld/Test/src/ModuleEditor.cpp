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

#include <filesystem>

#include "GeometryGenerator.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "ModuleScene.h"
#include "Texture.h"
#include "ModelImporter.h"
#include "MeshImporter.h"
#include "TextureImporter.h"
#include "MetaFile.h"


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
        gameObject->UpdateAABB();
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

    // Click izquierdo para seleccionar
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        app.moduleScene->SetSelectedGameObject(go);
        ModuleEditor::PushEnginePrintf("Selected GameObject: %s", go->GetName());
    }

    // Click derecho para menu contextual (estilo Unity)
    if (ImGui::BeginPopupContextItem())
    {
        // Auto-seleccionar al abrir el menu
        if (app.moduleScene->GetSelectedGameObject() != go)
        {
            app.moduleScene->SetSelectedGameObject(go);
        }

        ImGui::Text("GameObject: %s", go->GetName());
        ImGui::Separator();

        if (ImGui::MenuItem("Delete", "Supr"))
        {
            std::string name = go->GetName();
            app.moduleScene->DestroyGameObject(go);
            ModuleEditor::PushEnginePrintf("GameObject deleted: %s", name.c_str());
        }

        if (ImGui::MenuItem("Duplicate", "Ctrl + D"))
        {
            ModuleEditor::PushEngineLog("Duplicate not implemented yet");
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Create Empty Child"))
        {
            static int childCounter = 0;
            std::string childName = "Child_" + std::to_string(++childCounter);
            GameObject* child = app.moduleScene->CreateGameObject(childName.c_str(), go);
            ModuleEditor::PushEnginePrintf("Created child: %s", childName.c_str());
        }

        ImGui::EndPopup();
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
                ComponentTransform* draggedTransform = draggedGO->GetComponent<ComponentTransform>();

                if (draggedTransform)
                {
                    glm::mat4 currentGlobalMatrix = draggedTransform->GetGlobalMatrix();
                    draggedGO->SetParent(go);
                    ComponentTransform* newParentTransform = go->GetComponent<ComponentTransform>();

                    if (newParentTransform)
                    {
                        glm::mat4 parentGlobalMatrix = newParentTransform->GetGlobalMatrix();
                        glm::mat4 newLocalMatrix = glm::inverse(parentGlobalMatrix) * currentGlobalMatrix;

                        glm::vec3 newLocalPos, newLocalScale, skew;
                        glm::vec4 perspective;
                        glm::quat newLocalRot;

                        glm::decompose(newLocalMatrix, newLocalScale, newLocalRot, newLocalPos, skew, perspective);

                        draggedTransform->SetPosition(newLocalPos);
                        draggedTransform->SetRotation(newLocalRot);
                        draggedTransform->SetScale(newLocalScale);

                        ModuleEditor::PushEnginePrintf("%s is now child of %s (position preserved)",
                            draggedGO->GetName(), go->GetName());
                    }
                    else
                    {
                        ModuleEditor::PushEnginePrintf("%s is now child of %s (WARNING: parent has no transform)",
                            draggedGO->GetName(), go->GetName());
                    }

                    app.moduleScene->UpdateAllAABBs();
                }
                else
                {
                    draggedGO->SetParent(go);
                    ModuleEditor::PushEnginePrintf("%s is now child of %s (no transform to preserve)",
                        draggedGO->GetName(), go->GetName());
                    app.moduleScene->UpdateAllAABBs();
                }
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

    show_viewport_window = true;
    show_hierarchy_window = true;
    show_inspector_window = true;

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

    std::filesystem::create_directories(scenes_directory);
    PushEnginePrintf("Scene directory ready: %s", scenes_directory.c_str());

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

    // ----- Dockspace host so windows can dock into the main viewport -----
    {
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(main_viewport->WorkPos);
        ImGui::SetNextWindowSize(main_viewport->WorkSize);
        ImGui::SetNextWindowViewport(main_viewport->ID);

        ImGuiWindowFlags host_flags = ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoNavFocus
            | ImGuiWindowFlags_NoDecoration;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("MainDockSpaceWindow", nullptr, host_flags);
        ImGui::PopStyleVar(2);

        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::End();
    }



    // ===== KEYBOARD SHORTCUTS =====
    ImGuiIO& io = ImGui::GetIO();

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
    {
        OpenSaveSceneDialog();
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false))
    {
        OpenLoadSceneDialog();
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N, false))
    {
        NewScene();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
    {
        auto& app = Application::GetInstance();
        if (app.moduleScene)
        {
            GameObject* selected = app.moduleScene->GetSelectedGameObject();
            if (selected)
            {
                std::string name = selected->GetName();
                app.moduleScene->DestroyGameObject(selected);
                PushEnginePrintf("GameObject deleted: %s", name.c_str());
            }
        }
    }

    if (show_save_scene_popup)
        ShowSaveScenePopup();

    if (show_load_scene_popup)
        ShowLoadScenePopup();

    // Main menu bar
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
            {
                OpenSaveSceneDialog();
            }

            if (ImGui::MenuItem("Load Scene", "Ctrl+O"))
            {
                OpenLoadSceneDialog();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("New Scene", "Ctrl+N"))
            {
                NewScene();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit", "Alt+F4"))
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
            bool prev_viewport = show_viewport_window;

            ImGui::MenuItem("About", NULL, &show_about_window);
            ImGui::MenuItem("Console", NULL, &show_console_window);
            ImGui::MenuItem("Hierarchy", NULL, &show_hierarchy_window);
            ImGui::MenuItem("Inspector", NULL, &show_inspector_window);
            ImGui::MenuItem("Viewport", NULL, &show_viewport_window);

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

            auto& app = Application::GetInstance();
            GameObject* selected = app.moduleScene ? app.moduleScene->GetSelectedGameObject() : nullptr;

            if (ImGui::MenuItem("Delete Selected", "Del", false, selected != nullptr))
            {
                if (selected)
                {
                    std::string name = selected->GetName();
                    app.moduleScene->DestroyGameObject(selected);
                    PushEnginePrintf("GameObject deleted: %s", name.c_str());
                }
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

    // ===== VIEWPORT WINDOW =====
    if (show_viewport_window)
    {
        // Posicion: centro, ocupa 60% del ancho, 70% del alto
        ImVec2 mainSize = ImGui::GetMainViewport()->Size;
        ImVec2 mainPos = ImGui::GetMainViewport()->Pos;

        float leftWidth = mainSize.x * 0.2f;
        float rightWidth = mainSize.x * 0.2f;
        float centerWidth = mainSize.x - leftWidth - rightWidth;
        float topHeight = mainSize.y * 0.7f;
        float bottomHeight = mainSize.y * 0.3f;

        ImGui::SetNextWindowPos(ImVec2(mainPos.x + leftWidth, mainPos.y + 20));
        ImGui::SetNextWindowSize(ImVec2(centerWidth, topHeight));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        if (ImGui::Begin("Scene Viewport", &show_viewport_window))
        {
            ImVec2 availableSize = ImGui::GetContentRegionAvail();
            viewportPos = ImGui::GetCursorScreenPos();
            viewportSize = availableSize;

            int newFBWidth = (int)availableSize.x;
            int newFBHeight = (int)availableSize.y;

            if (newFBWidth > 0 && newFBHeight > 0 &&
                (newFBWidth != sceneFBWidth || newFBHeight != sceneFBHeight))
            {
                sceneFBWidth = newFBWidth;
                sceneFBHeight = newFBHeight;

                glBindTexture(GL_TEXTURE_2D, sceneTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sceneFBWidth, sceneFBHeight,
                    0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

                glBindRenderbuffer(GL_RENDERBUFFER, sceneRBO);
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                    sceneFBWidth, sceneFBHeight);
            }

            if (sceneFramebuffer != 0 && Application::GetInstance().moduleScene &&
                sceneFBWidth > 0 && sceneFBHeight > 0)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, sceneFramebuffer);
                glViewport(0, 0, sceneFBWidth, sceneFBHeight);
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                auto& app = Application::GetInstance();
                if (app.camera && sceneFBWidth > 0 && sceneFBHeight > 0)
                {
                    float aspect = (float)sceneFBWidth / (float)sceneFBHeight;
                    app.camera->setProjection(45.0f, aspect, 0.1f, 1000.0f);
                }

                Application::GetInstance().moduleScene->RenderScene();
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }

            ImVec2 imagePos = ImGui::GetCursorScreenPos();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddImage(
                (ImTextureID)(intptr_t)sceneTexture,
                imagePos,
                ImVec2(imagePos.x + availableSize.x, imagePos.y + availableSize.y),
                ImVec2(0, 1),
                ImVec2(1, 0)
            );

            ImGui::Dummy(availableSize);

            bool imageHovered = ImGui::IsItemHovered();
            bool imageClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
            isMouseOverViewport = imageHovered;

            auto& app = Application::GetInstance();
            if (app.input)
            {
                app.input->SetViewportHovered(imageHovered);
            }

            HandleGizmo();

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
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    // Hierarchy window - IZQUIERDA
    if (show_hierarchy_window)
    {
        ImVec2 mainSize = ImGui::GetMainViewport()->Size;
        ImVec2 mainPos = ImGui::GetMainViewport()->Pos;

        float leftWidth = mainSize.x * 0.2f;
        float fullHeight = mainSize.y - 20;

        ImGui::SetNextWindowPos(ImVec2(mainPos.x, mainPos.y + 20));
        ImGui::SetNextWindowSize(ImVec2(leftWidth, fullHeight));

        ImGui::Begin("Hierarchy", &show_hierarchy_window);

        auto& app = Application::GetInstance();
        if (app.moduleScene)
        {
            const std::vector<GameObject*>& all = app.moduleScene->GetAllGameObjects();

            for (GameObject* go : all)
            {
                if (!go) continue;
                if (go->GetParent() == nullptr)
                {
                    DrawGameObjectNode(go, app);
                }
            }

            // Menu contextual en area vacia (para crear nuevos GameObjects)
            if (ImGui::BeginPopupContextWindow("HierarchyBgContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                if (ImGui::MenuItem("Create Empty"))
                {
                    CreateEmptyGameObject();
                    ModuleEditor::PushEngineLog("Created empty GameObject");
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Cube"))
                {
                    CreateGeometryGameObject("Cube");
                }
                if (ImGui::MenuItem("Sphere"))
                {
                    CreateGeometryGameObject("Sphere");
                }
                if (ImGui::MenuItem("Cylinder"))
                {
                    CreateGeometryGameObject("Cylinder");
                }
                if (ImGui::MenuItem("Pyramid"))
                {
                    CreateGeometryGameObject("Pyramid");
                }
                if (ImGui::MenuItem("Plane"))
                {
                    CreateGeometryGameObject("Plane");
                }

                ImGui::EndPopup();
            }
        }
        else
        {
            ImGui::Text("ModuleScene not available");
        }

        ImGui::End();
    }

    // Inspector window - DERECHA
    if (show_inspector_window)
    {
        ImVec2 mainSize = ImGui::GetMainViewport()->Size;
        ImVec2 mainPos = ImGui::GetMainViewport()->Pos;

        float leftWidth = mainSize.x * 0.2f;
        float centerWidth = mainSize.x * 0.6f;
        float rightWidth = mainSize.x * 0.2f;
        float fullHeight = mainSize.y - 20;

        ImGui::SetNextWindowPos(ImVec2(mainPos.x + leftWidth + centerWidth, mainPos.y + 20));
        ImGui::SetNextWindowSize(ImVec2(rightWidth, fullHeight));

        ImGui::Begin("Inspector", &show_inspector_window);

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

                    const char* alphaModeNames[] = { "Opaque", "Alpha Test", "Alpha Blend" };
                    int currentMode = (int)mat->GetAlphaMode();
                    if (ImGui::Combo("Alpha Mode", &currentMode, alphaModeNames, 3))
                    {
                        mat->SetAlphaMode((AlphaMode)currentMode);
                        PushEnginePrintf("Alpha mode changed to: %s", alphaModeNames[currentMode]);
                    }

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

    // Console window - ABAJO
    if (show_console_window)
    {
        ImVec2 mainSize = ImGui::GetMainViewport()->Size;
        ImVec2 mainPos = ImGui::GetMainViewport()->Pos;

        float leftWidth = mainSize.x * 0.2f;
        float centerWidth = mainSize.x * 0.6f;
        float topHeight = mainSize.y * 0.7f;
        float bottomHeight = mainSize.y * 0.3f - 20;

        ImGui::SetNextWindowPos(ImVec2(mainPos.x + leftWidth, mainPos.y + topHeight + 20));
        ImGui::SetNextWindowSize(ImVec2(centerWidth, bottomHeight));

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
            std::string droppedPath(data);
            std::string ext = GetFileExtension(droppedPath);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            PushEnginePrintf("File dropped: %s", droppedPath.c_str());

            auto& app = Application::GetInstance();

            // NO cargar directamente archivos .wzd, .wzm, .wzt
            // Estos son archivos de Library, no deben ser arrastrados
            if (ext == "wzd" || ext == "wzm" || ext == "wzt") {
                PushEngineLog("ERROR: Library files should not be imported directly!");
                PushEngineLog("Please drag the original source file (FBX, PNG, etc.)");
                return;
            }

            // Copiar a Assets/
            std::filesystem::path sourcePath(droppedPath);
            std::string targetPath = "Assets/" + sourcePath.filename().string();

            try {
                // Check if already exists
                if (std::filesystem::exists(targetPath)) {
                    PushEnginePrintf("WARNING: File already exists in Assets: %s", targetPath.c_str());

                    // Check if needs reimport
                    if (app.assetManager && app.assetManager->NeedsReimport(targetPath)) {
                        PushEngineLog("File has been modified, reimporting...");
                    }
                    else {
                        PushEngineLog("File is up-to-date, loading from cache...");

                        // Load directly from library
                        std::string libraryPath = app.assetManager->GetLibraryPath(targetPath);
                        if (!libraryPath.empty()) {
                            if (ext == "fbx" || ext == "obj" || ext == "gltf" ||
                                ext == "glb" || ext == "dae") {
                                LoadModelFromWZD(libraryPath);
                            }
                        }
                        return;
                    }
                }

                std::filesystem::copy_file(droppedPath, targetPath,
                    std::filesystem::copy_options::overwrite_existing);

                PushEnginePrintf("Copied to: %s", targetPath.c_str());

                // Process with AssetManager
                if (app.assetManager)
                {
                    if (app.assetManager->ProcessAssetFile(targetPath))
                    {
                        PushEngineLog("Asset imported successfully!");
                        app.assetManager->PrintStatistics();

                        // Load model automatically
                        if (ext == "fbx" || ext == "obj" || ext == "gltf" ||
                            ext == "glb" || ext == "dae")
                        {
                            std::string libraryPath = app.assetManager->GetLibraryPath(targetPath);
                            if (!libraryPath.empty())
                            {
                                // Get meta data for import settings
                                WizardEngine::AssetMetaData* metaData = app.assetManager->GetMetaData(targetPath);
                                LoadModelFromWZD(libraryPath, metaData);
                            }
                        }
                    }
                    else
                    {
                        PushEngineLog("ERROR: Failed to import asset");
                    }
                }
            }
            catch (const std::exception& e) {
                PushEnginePrintf("ERROR copying file: %s", e.what());
            }
        }
    }

    ImGui_ImplSDL3_ProcessEvent(&event);
}

// Helper para obtener extensión
std::string ModuleEditor::GetFileExtension(const std::string& filepath)
{
    size_t pos = filepath.find_last_of('.');
    if (pos != std::string::npos && pos + 1 < filepath.size())
    {
        return filepath.substr(pos + 1);
    }
    return "";
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

void ModuleEditor::RefreshSceneList()
{
    available_scenes.clear();

    // Crear directorio si no existe
    std::filesystem::create_directories(scenes_directory);

    // Buscar archivos .json en el directorio
    for (const auto& entry : std::filesystem::directory_iterator(scenes_directory))
    {
        if (entry.is_regular_file())
        {
            std::string filename = entry.path().filename().string();
            std::string ext = entry.path().extension().string();

            // Solo archivos .json
            if (ext == ".json")
            {
                available_scenes.push_back(filename);
            }
        }
    }

    PushEnginePrintf("Found %zu scene(s) in %s", available_scenes.size(), scenes_directory.c_str());
}

void ModuleEditor::ShowSaveScenePopup()
{
    ImGui::OpenPopup("Save Scene");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Save Scene", &show_save_scene_popup, ImGuiWindowFlags_NoResize))
    {
        ImGui::Text("Enter scene name:");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::InputText("##filename", scene_filename_buffer, sizeof(scene_filename_buffer));

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Location: %s", scenes_directory.c_str());
        ImGui::Spacing();

        ImGui::Separator();

        // Boton SAVE
        if (ImGui::Button("Save", ImVec2(120, 0)))
        {
            std::string filename = scene_filename_buffer;

            // Anadir extension .json si no la tiene
            if (filename.find(".json") == std::string::npos)
            {
                filename += ".json";
            }

            std::string fullPath = scenes_directory + filename;

            auto& app = Application::GetInstance();
            if (app.moduleScene)
            {
                if (app.moduleScene->SaveScene(fullPath))
                {
                    PushEnginePrintf("Scene saved successfully: %s", filename.c_str());
                }
                else
                {
                    PushEnginePrintf("ERROR: Failed to save scene: %s", filename.c_str());
                }
            }

            show_save_scene_popup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        // Boton CANCEL
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            show_save_scene_popup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void ModuleEditor::ShowLoadScenePopup()
{
    ImGui::OpenPopup("Load Scene");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Load Scene", &show_load_scene_popup))
    {
        ImGui::Text("Select a scene to load:");
        ImGui::Separator();
        ImGui::Spacing();

        // Lista de escenas
        ImGui::BeginChild("SceneListRegion", ImVec2(0, 280), true);
        {
            if (available_scenes.empty())
            {
                ImGui::TextDisabled("No scenes found in %s", scenes_directory.c_str());
            }
            else
            {
                for (size_t i = 0; i < available_scenes.size(); i++)
                {
                    std::string& sceneName = available_scenes[i];

                    ImGui::PushID(i);

                    // Mostrar nombre del archivo
                    if (ImGui::Selectable(sceneName.c_str(), false, ImGuiSelectableFlags_DontClosePopups))
                    {
                        // Copiar nombre al buffer
                        strncpy(scene_filename_buffer, sceneName.c_str(), sizeof(scene_filename_buffer) - 1);
                        scene_filename_buffer[sizeof(scene_filename_buffer) - 1] = '\0';
                    }

                    // Doble click para cargar directamente
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        std::string fullPath = scenes_directory + sceneName;

                        auto& app = Application::GetInstance();
                        if (app.moduleScene)
                        {
                            if (app.moduleScene->LoadScene(fullPath))
                            {
                                PushEnginePrintf("Scene loaded successfully: %s", sceneName.c_str());
                                app.moduleScene->UpdateAllAABBs();
                            }
                            else
                            {
                                PushEnginePrintf("ERROR: Failed to load scene: %s", sceneName.c_str());
                            }
                        }

                        show_load_scene_popup = false;
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::PopID();
                }
            }
        }
        ImGui::EndChild();

        ImGui::Spacing();
        ImGui::Text("Selected: %s", scene_filename_buffer);
        ImGui::Spacing();
        ImGui::Separator();

        // Boton LOAD
        if (ImGui::Button("Load", ImVec2(120, 0)))
        {
            std::string filename = scene_filename_buffer;
            std::string fullPath = scenes_directory + filename;

            auto& app = Application::GetInstance();
            if (app.moduleScene)
            {
                if (app.moduleScene->LoadScene(fullPath))
                {
                    PushEnginePrintf("Scene loaded successfully: %s", filename.c_str());
                    app.moduleScene->UpdateAllAABBs();
                }
                else
                {
                    PushEnginePrintf("ERROR: Failed to load scene: %s", filename.c_str());
                }
            }

            show_load_scene_popup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        // Boton REFRESH
        if (ImGui::Button("Refresh", ImVec2(120, 0)))
        {
            RefreshSceneList();
        }

        ImGui::SameLine();

        // Boton CANCEL
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            show_load_scene_popup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void ModuleEditor::NewScene()
{
    auto& app = Application::GetInstance();
    if (app.moduleScene)
    {
        app.moduleScene->ClearScene();
        GameObject* newRoot = app.moduleScene->CreateGameObject("Scene Root", nullptr);
        app.moduleScene->SetRoot(newRoot);
        PushEngineLog("New scene created");
    }
}

void ModuleEditor::OpenSaveSceneDialog()
{
    show_save_scene_popup = true;
    PushEngineLog("Opening Save Scene dialog...");
}

void ModuleEditor::OpenLoadSceneDialog()
{
    RefreshSceneList();
    show_load_scene_popup = true;
    PushEngineLog("Opening Load Scene dialog...");
}

void ModuleEditor::LoadModelFromWZD(const std::string& wzdPath, WizardEngine::AssetMetaData* metaData)
{
    PushEnginePrintf("Loading model from: %s", wzdPath.c_str());

    auto& app = Application::GetInstance();
    if (!app.moduleScene)
    {
        PushEngineLog("ERROR: ModuleScene not available");
        return;
    }

    // Load model custom
    WizardEngine::WizardModelData modelData;
    if (!WizardEngine::ModelImporter::Load(wzdPath, modelData))
    {
        PushEngineLog("ERROR: Failed to load WZD file");
        return;
    }

    // Create GameObject root for model
    std::string modelName = std::filesystem::path(wzdPath).stem().string();
    GameObject* modelRoot = app.moduleScene->CreateGameObject(
        modelName.c_str(),
        app.moduleScene->GetRoot()
    );

    if (!modelRoot)
    {
        PushEngineLog("ERROR: Failed to create GameObject");
        return;
    }

    // Apply import settings from meta
    ComponentTransform* rootTransform = modelRoot->GetComponent<ComponentTransform>();
    if (rootTransform && metaData)
    {
        rootTransform->SetScale(metaData->importScale);
        rootTransform->SetRotation(glm::quat(glm::radians(metaData->importRotation)));
        rootTransform->SetPosition(metaData->importPosition);

        PushEnginePrintf("Applied import settings - Scale: (%.2f, %.2f, %.2f)",
            metaData->importScale.x, metaData->importScale.y, metaData->importScale.z);
    }

    // Load each mesh
    for (size_t i = 0; i < modelData.meshes.size(); i++)
    {
        const auto& meshRef = modelData.meshes[i];

        WizardEngine::WizardMeshData meshData;
        if (!WizardEngine::MeshImporter::Load(meshRef.meshFilepath, meshData))
        {
            PushEnginePrintf("WARNING: Failed to load mesh: %s", meshRef.meshFilepath.c_str());
            continue;
        }

        std::string meshName = modelName + "_mesh_" + std::to_string(i);
        GameObject* meshObj = app.moduleScene->CreateGameObject(meshName.c_str(), modelRoot);

        if (!meshObj) continue;

        // Add ComponentMesh
        ComponentMesh* meshComp = static_cast<ComponentMesh*>(
            meshObj->CreateComponent(ComponentType::MESH)
            );

        if (meshComp)
        {
            MeshGeometry geom;
            geom.vertices.reserve(meshData.vertices.size());

            for (const auto& v : meshData.vertices)
            {
                GeomVertex gv;
                gv.Position = v.position;
                gv.Normal = v.normal;
                gv.TexCoords = v.texCoords;
                geom.vertices.push_back(gv);
            }

            geom.indices = meshData.indices;

            meshComp->LoadFromGeometry(&geom);
        }

        // Add ComponentMaterial
        ComponentMaterial* matComp = static_cast<ComponentMaterial*>(
            meshObj->CreateComponent(ComponentType::MATERIAL)
            );

        if (matComp && meshRef.materialIndex < modelData.materials.size())
        {
            const auto& matData = modelData.materials[meshRef.materialIndex];

            if (!matData.diffuseTexture.empty())
            {
                WizardEngine::WizardTextureData texData;
                if (WizardEngine::TextureImporter::Load(matData.diffuseTexture, texData))
                {
                    GLuint texID;
                    glGenTextures(1, &texID);
                    glBindTexture(GL_TEXTURE_2D, texID);

                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                        texData.width, texData.height, 0,
                        GL_RGBA, GL_UNSIGNED_BYTE, texData.data.data());

                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    matComp->SetTexture(texID, matData.diffuseTexture.c_str(), texData.channels);

                    PushEnginePrintf("Loaded texture: %s", matData.diffuseTexture.c_str());
                }
                else
                {
                    GLuint checkerTex = Texture::CreateCheckerboardTexture(512, 512, 32);
                    matComp->SetTexture(checkerTex, "checkerboard_default", 3);
                }
            }
            else
            {
                GLuint checkerTex = Texture::CreateCheckerboardTexture(512, 512, 32);
                matComp->SetTexture(checkerTex, "checkerboard_default", 3);
            }
        }

        meshObj->UpdateAABB();
    }

    app.moduleScene->UpdateAllAABBs();

    PushEnginePrintf("Model loaded successfully: %s (%zu meshes)",
        modelName.c_str(), modelData.meshes.size());
}

void ModuleEditor::LoadMeshFromWZM(const std::string& wzmPath)
{
    PushEnginePrintf("Loading mesh from: %s", wzmPath.c_str());

    auto& app = Application::GetInstance();
    if (!app.moduleScene)
    {
        PushEngineLog("ERROR: ModuleScene not available");
        return;
    }

    // Cargar mesh custom
    WizardEngine::WizardMeshData meshData;
    if (!WizardEngine::MeshImporter::Load(wzmPath, meshData))
    {
        PushEngineLog("ERROR: Failed to load WZM file");
        return;
    }

    // Crear GameObject
    std::string meshName = std::filesystem::path(wzmPath).stem().string();
    GameObject* meshObj = app.moduleScene->CreateGameObject(
        meshName.c_str(),
        app.moduleScene->GetRoot()
    );

    if (!meshObj)
    {
        PushEngineLog("ERROR: Failed to create GameObject");
        return;
    }

    // Añadir ComponentMesh
    ComponentMesh* meshComp = static_cast<ComponentMesh*>(
        meshObj->CreateComponent(ComponentType::MESH)
        );

    if (meshComp)
    {
        // Convertir WizardMeshData a MeshGeometry
        MeshGeometry geom;
        geom.vertices.reserve(meshData.vertices.size());

        for (const auto& v : meshData.vertices)
        {
            GeomVertex gv;
            gv.Position = v.position;
            gv.Normal = v.normal;
            gv.TexCoords = v.texCoords;
            geom.vertices.push_back(gv);
        }

        geom.indices = meshData.indices;

        meshComp->LoadFromGeometry(&geom);
    }

    // Añadir material con checkerboard
    ComponentMaterial* matComp = static_cast<ComponentMaterial*>(
        meshObj->CreateComponent(ComponentType::MATERIAL)
        );

    if (matComp)
    {
        GLuint checkerTex = Texture::CreateCheckerboardTexture(512, 512, 32);
        matComp->SetTexture(checkerTex, "checkerboard_default", 3);
    }

    meshObj->UpdateAABB();
    app.moduleScene->UpdateAllAABBs();

    PushEnginePrintf("Mesh loaded successfully: %s", meshName.c_str());
}