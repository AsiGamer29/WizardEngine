#include "ModuleEditor.h"
#include "Application.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <sstream>
#include <cstdarg>

// Engine console storage definitions
std::vector<std::string> ModuleEditor::engine_log;
std::mutex ModuleEditor::engine_log_mutex;
size_t ModuleEditor::engine_log_max_messages = 8192;
bool ModuleEditor::engine_log_auto_scroll = true;

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
                // Signal application to quit via SDL event
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
            // track toggles to inform
            bool prev_demo = show_demo_window;
            bool prev_test = show_test_window;
            bool prev_about = show_about_window;
            bool prev_console = show_console_window;

            ImGui::MenuItem("Demo Window", NULL, &show_demo_window);
            ImGui::MenuItem("Test Window", NULL, &show_test_window);
            ImGui::MenuItem("About", NULL, &show_about_window);
            ImGui::MenuItem("Console", NULL, &show_console_window);

            if (prev_demo != show_demo_window)
                PushEnginePrintf("Demo Window %s", show_demo_window ? "opened" : "closed");
            if (prev_test != show_test_window)
                PushEnginePrintf("Test Window %s", show_test_window ? "opened" : "closed");
            if (prev_about != show_about_window)
                PushEnginePrintf("About Window %s", show_about_window ? "opened" : "closed");
            if (prev_console != show_console_window)
                PushEnginePrintf("Console Window %s", show_console_window ? "opened" : "closed");

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

        if (ImGui::BeginMenu("Geometry"))
        {
            if (ImGui::MenuItem("Cube"))
            {
                requested_geometry = "Cube";
                PushEnginePrintf("Requested geometry: %s", requested_geometry.c_str());
                if (Application::GetInstance().opengl)
                {
                    Application::GetInstance().opengl->LoadGeometry("Cube");
                    PushEnginePrintf("LoadGeometry called for %s", "Cube");
                }
            }
            if (ImGui::MenuItem("Sphere"))
            {
                requested_geometry = "Sphere";
                PushEnginePrintf("Requested geometry: %s", requested_geometry.c_str());
                if (Application::GetInstance().opengl)
                {
                    Application::GetInstance().opengl->LoadGeometry("Sphere");
                    PushEnginePrintf("LoadGeometry called for %s", "Sphere");
                }
            }
            if (ImGui::MenuItem("Cylinder"))
            {
                requested_geometry = "Cylinder";
                PushEnginePrintf("Requested geometry: %s", requested_geometry.c_str());
                if (Application::GetInstance().opengl)
                {
                    Application::GetInstance().opengl->LoadGeometry("Cylinder");
                    PushEnginePrintf("LoadGeometry called for %s", "Cylinder");
                }
            }
            if (ImGui::MenuItem("Pyramid"))
            {
                requested_geometry = "Pyramid";
                PushEnginePrintf("Requested geometry: %s", requested_geometry.c_str());
                if (Application::GetInstance().opengl)
                {
                    Application::GetInstance().opengl->LoadGeometry("Pyramid");
                    PushEnginePrintf("LoadGeometry called for %s", "Pyramid");
                }
            }
            if (ImGui::MenuItem("Plane"))
            {
                requested_geometry = "Plane";
                PushEnginePrintf("Requested geometry: %s", requested_geometry.c_str());
                if (Application::GetInstance().opengl)
                {
                    Application::GetInstance().opengl->LoadGeometry("Plane");
                    PushEnginePrintf("LoadGeometry called for %s", "Plane");
                }
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

        // Show last requested geometry (for debugging / future connection)
        if (!requested_geometry.empty())
        {
            ImGui::Separator();
            ImGui::Text("Last requested geometry: %s", requested_geometry.c_str());
        }

        ImGui::End();
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

        ImGui::Text("CPU Count: %d", SDL_GetNumLogicalCPUCores());
        ImGui::Text("CPU Cache Line Size: %d bytes", SDL_GetCPUCacheLineSize());
        ImGui::Text("System RAM (MB): %d", SDL_GetSystemRAM());

        const char* glver2 = (const char*)glGetString(GL_VERSION);
        ImGui::Text("OpenGL Version: %s", glver2 ? glver2 : "Unknown");

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
            // extract filename (basename)
            std::string name;
            size_t pos = path.find_last_of("/\\");
            if (pos != std::string::npos && pos + 1 < path.size())
                name = path.substr(pos + 1);
            else
                name = path;

            // determine extension
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