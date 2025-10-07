#include "Engine.h"
#include <iostream>
#include <sstream>
#include <iomanip>

#include "Window.h"
#include "Input.h"

// Constructor
Engine::Engine() {

    frames = 0;

    window = std::make_shared<Window>();
    input = std::make_shared<Input>();


    AddModule(std::static_pointer_cast<Module>(window));
    AddModule(std::static_pointer_cast<Module>(input));

}

// Static method to get the instance of the Engine class, following the singleton pattern
Engine& Engine::GetInstance() {
    static Engine instance;
    return instance;
}

void Engine::AddModule(std::shared_ptr<Module> module) {
    module->Init();
    moduleList.push_back(module);
}

bool Engine::Awake() {

    return true;
}

bool Engine::Start() {

    bool result = true;
    for (const auto& module : moduleList) {
        result = module.get()->Start();
        if (!result) {
            break;
        }
    }

    return result;
}

bool Engine::Update() {
   
    bool ret = true;
    PrepareUpdate();

    if (input->GetWindowEvent(WE_QUIT) == true)
        ret = false;

    if (ret == true)
        ret = PreUpdate();

    if (ret == true)
        ret = DoUpdate();

    if (ret == true)
        ret = PostUpdate();

    FinishUpdate();

    return ret;
}

bool Engine::CleanUp() {

    bool result = true;
    for (const auto& module : moduleList) {
        result = module.get()->CleanUp();
        if (!result) {
            break;
        }
    }

    return result;
}

void Engine::FinishUpdate()
{
    frameCount++;
    lastSecFrameCount++;
}

bool Engine::PreUpdate()
{
    bool result = true;
    for (const auto& module : moduleList) {
        result = module.get()->PreUpdate();
        if (!result) {
            break;
        }
    }

    return result;
}

bool Engine::DoUpdate()
{
    bool result = true;
    for (const auto& module : moduleList) {
        result = module.get()->Update(dt);
        if (!result) {
            break;
        }
    }

    return result;
}

bool Engine::PostUpdate()
{
    bool result = true;
    for (const auto& module : moduleList) {
        result = module.get()->PostUpdate();
        if (!result) {
            break;
        }
    }

    return result;
}
