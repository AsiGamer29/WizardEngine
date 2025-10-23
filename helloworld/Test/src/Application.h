#pragma once

#include <memory>
#include <list>
#include "Window.h"
#include "Module.h"
#include "Input.h"
#include "OpenGL.h"
#include "Model.h"
#include "Shader.h"

class Module;

class Application
{
public:
    static Application& GetInstance();

    bool Awake();
    bool Start();
    bool Update();
    bool CleanUp();

    std::shared_ptr<Window> window;
    std::shared_ptr<Input> input;
    std::shared_ptr<OpenGL> opengl;

private:
    Application();
    ~Application() = default;
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void AddModule(std::shared_ptr<Module> module);
    std::list<std::shared_ptr<Module>> moduleList;

    bool isRunning;

    bool PreUpdate();
    bool DoUpdate();
    bool PostUpdate();

    // Recursos
    /*std::unique_ptr<Model> myModel;*/
    std::unique_ptr<Shader> myShader;
};
