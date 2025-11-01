#pragma once
#include "Module.h"
#include <SDL3/SDL.h>
#include <string>

class ModuleEditor : public Module
{
public:
    ModuleEditor();
    ~ModuleEditor() override;

    bool Start() override;
    bool PreUpdate() override;
    bool Update() override;
    bool PostUpdate() override;
    bool CleanUp() override;

    void ProcessEvent(const SDL_Event& event);

private:
    bool show_demo_window = true;
    bool show_test_window = true;
    bool show_about_window = false; // About window toggle

    // Geometry loading menu helper
    std::string requested_geometry; // name of the geometry requested to load

    // Configuration windows
    bool show_config_performance = false;
    bool show_config_modules = false;
    bool show_config_system = false;

    // FPS history for graph
    static constexpr int FPS_HISTORY_SIZE = 120;
    float fps_history[FPS_HISTORY_SIZE];
    int fps_pos = 0;
    int fps_count = 0;

    // Module settings placeholders
    struct ModuleSettings
    {
        // Window
        int window_width = 1280;
        int window_height = 720;
        bool vsync = true;
        // Renderer
        bool wireframe = false;
        float clear_color[3] = {0.1f, 0.1f, 0.1f};
        // Input
        float mouse_sensitivity = 1.0f;
        // Textures
        int texture_filter = 0; // 0 = Nearest, 1 = Linear
    } settings;
};