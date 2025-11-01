#include "ModuleEditor.h"
#include "Application.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <SDL3/SDL.h>

ModuleEditor::ModuleEditor()
{
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
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Documentation on GitHub"))
            {
                // placeholder URL, user will replace
                SDL_OpenURL("https://github.com/your/repo/docs");
            }
            if (ImGui::MenuItem("Report a bug"))
            {
                SDL_OpenURL("https://github.com/your/repo/issues");
            }
            if (ImGui::MenuItem("Download latest"))
            {
                SDL_OpenURL("https://github.com/your/repo/releases");
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
            ImGui::BulletText("Saüc");
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