#pragma once
#include "Module.h"
#include "MetaFile.h"
#include "Resource.h"
#include "ModuleResources.h"
#include "imgui.h"
#include <glad/glad.h>
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
    void HandleGizmo();

    static void PushEngineLog(const std::string& msg);
    static void PushEnginePrintf(const char* fmt, ...);

    GLuint sceneFramebuffer = 0;
    GLuint sceneTexture = 0;
    GLuint sceneRBO = 0;
    int sceneFBWidth = 1280;
    int sceneFBHeight = 720;

    bool show_viewport_window = true;
    bool IsMouseOverViewport() const { return isMouseOverViewport; }

private:
    bool show_demo_window = false;
    bool show_about_window = false;
    bool show_console_window = true;
    bool show_hierarchy_window = true;
    bool show_inspector_window = true;
    bool show_asset_browser = true;
    bool show_config_resources = false;


    bool firstTimeLayout = true;

    // NUEVO: Variables para guardar/cargar escenas
    bool show_save_scene_popup = false;
    bool show_load_scene_popup = false;
    char scene_filename_buffer[256] = "MyScene.json";
    std::string scenes_directory = "Assets/Scenes/";

    std::vector<std::string> available_scenes;
    void RefreshSceneList();
    void ShowSaveScenePopup();
    void ShowLoadScenePopup();
    void NewScene();
    void OpenSaveSceneDialog();
    void OpenLoadSceneDialog();

    void LoadModelFromWZD(const std::string& wzdPath, WizardEngine::AssetMetaData* metaData = nullptr);
    void LoadMeshFromWZM(const std::string& wzmPath);
    std::string GetFileExtension(const std::string& filepath);
    void LoadModelFromAssetBrowser(const std::string& libraryPath, WizardEngine::AssetMetaData* metaData, const std::string& originalAssetPath);

    bool inspector_show_checkerboard = false;
    unsigned int inspectorCheckerTex = 0;
    void* inspectorOverrideTarget = nullptr;

    std::string requested_geometry;

    bool show_config_performance = false;
    bool show_config_modules = false;
    bool show_config_system = false;

    static constexpr int FPS_HISTORY_SIZE = 120;
    float fps_history[FPS_HISTORY_SIZE];
    int fps_pos = 0;
    int fps_count = 0;

    bool isMouseOverViewport = false;
    ImVec2 viewportPos;
    ImVec2 viewportSize;

    enum class GizmoOperation
    {
        TRANSLATE,
        ROTATE,
        SCALE
    };

    enum class GizmoMode
    {
        LOCAL,
        WORLD
    };

    GizmoOperation currentGizmoOperation = GizmoOperation::TRANSLATE;
    GizmoMode currentGizmoMode = GizmoMode::WORLD;
    bool useSnap = false;
    float snapValues[3] = { 1.0f, 15.0f, 0.1f };

    struct ModuleSettings
    {
        int window_width = 1280;
        int window_height = 720;
        bool vsync = true;
        bool wireframe = false;
        float clear_color[3] = { 0.1f, 0.1f, 0.1f };
        float mouse_sensitivity = 1.0f;
        int texture_filter = 0;
    } settings;

    static std::vector<std::string> engine_log;
    static std::mutex engine_log_mutex;
    static size_t engine_log_max_messages;
    static bool engine_log_auto_scroll;

    // Asset Browser
    std::string currentAssetPath = "../Assets/";
    std::string selectedAssetPath;

    void DrawAssetBrowser();
    void DrawFolderTree(const std::filesystem::path& path, const std::filesystem::path& currentPath);
    void DrawAssetGrid();
    const char* GetIconForFile(const std::string& extension);
    ImVec4 GetColorForFileType(const std::string& extension);
};