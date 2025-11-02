#pragma once
#include "Module.h"
#include <string>
#include <vector>
#include <mutex>

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

    // API for engine modules to report messages to the editor console
    static void PushEngineLog(const std::string& msg);
    static void PushEnginePrintf(const char* fmt, ...);

private:
    bool show_demo_window = true;
    bool show_test_window = true;
    bool show_about_window = false; // About window toggle
    bool show_console_window = false; // Console window toggle
    bool show_hierarchy_window = true; // Jerarquía window toggle
    bool show_inspector_window = true; // Inspector window toggle (new)

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

    // Engine console storage (thread-safe)
    static std::vector<std::string> engine_log;
    static std::mutex engine_log_mutex;
    static size_t engine_log_max_messages;
    static bool engine_log_auto_scroll;

};