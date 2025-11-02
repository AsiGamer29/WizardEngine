#pragma once
#include "Module.h"
#include <string>

// Forward declaration for SDL
union SDL_Event;

class ModuleEditor : public Module
{
public:
    ModuleEditor();
    ~ModuleEditor();

    bool Start() override;
    bool PreUpdate() override;
    bool Update() override;
    bool PostUpdate() override;
    bool CleanUp() override;

    void ProcessEvent(const SDL_Event& event);

private:
    // Window visibility flags
    bool show_demo_window = false;
    bool show_test_window = false;
    bool show_about_window = false;
    bool show_config_performance = false;
    bool show_config_modules = false;
    bool show_config_system = false;

    // FPS tracking
    static const int FPS_HISTORY_SIZE = 100;
    float fps_history[FPS_HISTORY_SIZE];
    int fps_pos;
    int fps_count;

    // Settings (could be moved to a separate structure)
    struct Settings
    {
        int window_width = 1280;
        int window_height = 720;
        bool vsync = true;
        bool wireframe = false;
        float clear_color[3] = { 0.1f, 0.1f, 0.1f };
        float mouse_sensitivity = 1.0f;
        int texture_filter = 1; // 0=nearest, 1=linear
    } settings;

    // For tracking last geometry request
    std::string requested_geometry;
};