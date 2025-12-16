#pragma once
#include "Module.h"
#include "Window.h"
#include "OpenGL.h"
#include "Input.h"
#include "ModuleEditor.h"
#include "Camera.h"
#include "ModuleScene.h"
#include "AssetManager.h"
#include "ModuleResources.h"
#include <memory>
#include <vector>

class Application
{
public:
    static Application& GetInstance();

    // Módulos
    std::shared_ptr<Window> window;
    std::shared_ptr<Input> input;
    std::shared_ptr<OpenGL> opengl;
    std::shared_ptr<ModuleEditor> editor;
    std::shared_ptr<Camera> camera;
    std::shared_ptr<ModuleScene> moduleScene;
    std::shared_ptr<WizardEngine::AssetManager> assetManager;
	std::shared_ptr<WizardEngine::ModuleResources> moduleResources;



    bool Awake();
    bool Start();
    bool Update();
    bool CleanUp();

    bool IsRunning() const { return isRunning; }
    void SetRunning(bool running) { isRunning = running; }

    std::shared_ptr<Camera> GetCamera() const { return camera; }
    std::shared_ptr<ModuleScene> GetModuleScene() const { return moduleScene; }
    WizardEngine::AssetManager* GetAssetManager();  
private:
    Application();
    ~Application() = default;
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void AddModule(std::shared_ptr<Module> module);
    bool PreUpdate();
    bool DoUpdate();
    bool PostUpdate();

    void AssignStreetTextures();

    std::vector<std::shared_ptr<Module>> moduleList;
    bool isRunning;
};