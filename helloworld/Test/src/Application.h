#ifndef __APPLICATION_H__
#define __APPLICATION_H__
#include <vector>
#include <memory>
#include "Module.h"
#include "Window.h"
#include "Input.h"
#include "OpenGL.h"
#include "ModuleEditor.h"
#include "Camera.h"

class Application
{
public:
    static Application& GetInstance();
    void AddModule(std::shared_ptr<Module> module);

    bool Awake();
    bool Start();
    bool Update();
    bool CleanUp();

    std::shared_ptr<Window> window;
    std::shared_ptr<Input> input;
    std::shared_ptr<OpenGL> opengl;
    std::shared_ptr<ModuleEditor> editor;
    std::shared_ptr<Camera> camera;

private:
    Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool PreUpdate();
    bool DoUpdate();
    bool PostUpdate();

    std::vector<std::shared_ptr<Module>> moduleList;
    bool isRunning;
};

#endif