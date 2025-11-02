#include "Application.h"
#include <iostream>

Application::Application() : isRunning(true)
{
    

    std::cout << "Application Constructor" << std::endl;
    window = std::make_shared<Window>();
    input = std::make_shared<Input>();
    opengl = std::make_shared<OpenGL>();
    editor = std::make_shared<ModuleEditor>();
    moduleScene = std::make_shared<ModuleScene>();
    camera = std::make_shared<Camera>(
        glm::vec3(0.0f, 0.0f, 5.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        -90.0f, 0.0f
    );
    
    AddModule(std::static_pointer_cast<Module>(window));
    AddModule(std::static_pointer_cast<Module>(input));
    AddModule(std::static_pointer_cast<Module>(opengl));
    AddModule(std::static_pointer_cast<Module>(moduleScene));
    AddModule(std::static_pointer_cast<Module>(editor));

}

Application& Application::GetInstance()
{
    static Application instance;
    return instance;
}

void Application::AddModule(std::shared_ptr<Module> module)
{
    moduleList.push_back(module);
}

bool Application::Awake()
{
    return true;
}

bool Application::Start()
{
    bool result = true;
    for (const auto& module : moduleList) {
        result = module.get()->Start();
        if (!result) {
            break;
        }
    }
    return result;
}

bool Application::Update()
{
    bool ret = true;

    if (input->GetWindowEvent(WE_QUIT) == true)
        ret = false;

    if (ret == true)
        ret = PreUpdate();

    if (ret == true)
        ret = DoUpdate();

    if (ret == true)
        ret = PostUpdate();

    return ret;
}

bool Application::PreUpdate()
{
    //El orden este es muy importante porque si lo cambias de sitio se renderizan cosas encima de otras y luego no se vera el imgui
    input->PreUpdate();
    opengl->PreUpdate();
    editor->PreUpdate();
    window->PreUpdate();

    return true;
}

bool Application::DoUpdate()
{
    bool result = true;
    for (const auto& module : moduleList) {
        result = module.get()->Update();
        if (!result) {
            break;
        }
    }

    if (camera && input)
    {
        float deltaTime = 0.016f;
        camera->update(input.get(), deltaTime);
    }

    return result;
}

bool Application::PostUpdate()
{
    opengl->PostUpdate();
    editor->PostUpdate();
    window->PostUpdate();
    input->PostUpdate();

    return true;
}

bool Application::CleanUp()
{
    std::cout << "Application CleanUp" << std::endl;

    bool result = true;
    for (const auto& module : moduleList) {
        result = module.get()->CleanUp();
        if (!result) {
            break;
        }
    }
    return result;
}