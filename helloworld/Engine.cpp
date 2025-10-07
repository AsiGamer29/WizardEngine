#include "Engine.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include "Log.h"

#include "Window.h"
#include "Input.h"
#include "Render.h"
#include "Textures.h"
#include "Audio.h"
#include "Scene.h"
#include "EntityManager.h"
#include "Map.h"
#include "Physics.h"
// #include "GuiManager.h" // Eliminado porque no está implementado

#include "tracy/Tracy.hpp"

// Constructor
Engine::Engine() {

    LOG("Constructor Engine::Engine");

    Timer timer = Timer();
    startupTime = Timer();
    frameTime = PerfTimer();
    lastSecFrameTime = PerfTimer();
    frames = 0;

    window = std::make_shared<Window>();
    input = std::make_shared<Input>();
    render = std::make_shared<Render>();
    textures = std::make_shared<Textures>();
    audio = std::make_shared<Audio>();
    physics = std::make_shared<Physics>();
    scene = std::make_shared<Scene>();
    map = std::make_shared<Map>();
    entityManager = std::make_shared<EntityManager>();
    // guiManager = std::make_shared<GuiManager>(); // Eliminado

    AddModule(std::static_pointer_cast<Module>(window));
    AddModule(std::static_pointer_cast<Module>(input));
    AddModule(std::static_pointer_cast<Module>(textures));
    AddModule(std::static_pointer_cast<Module>(audio));
    AddModule(std::static_pointer_cast<Module>(physics));
    AddModule(std::static_pointer_cast<Module>(map));
    AddModule(std::static_pointer_cast<Module>(scene));
    AddModule(std::static_pointer_cast<Module>(entityManager));
    // AddModule(std::static_pointer_cast<Module>(guiManager)); // Eliminado

    AddModule(std::static_pointer_cast<Module>(render));

    LOG("Timer App Constructor: %f", timer.ReadMSec());
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
    Timer timer = Timer();

    LOG("Engine::Awake");

    if (!LoadConfig()) {
        LOG("Error: No se pudo cargar el archivo de configuración.");
        return false;
    }

    auto engineNode = configFile.child("config").child("engine");
    gameTitle = engineNode.child("title").child_value();

    auto maxFrameAttr = engineNode.child("maxFrameDuration").attribute("value");
    if (maxFrameAttr) {
        maxFrameDuration = maxFrameAttr.as_int();
    }
    else {
        maxFrameDuration = 16; // Valor por defecto
    }

    bool result = true;
    for (const auto& module : moduleList) {
        module.get()->LoadParameters(configFile.child("config").child(module.get()->name.c_str()));
        result = module.get()->Awake();
        if (!result) {
            break;
        }
    }

    LOG("Timer App Awake(): %f", timer.ReadMSec());

    return result;
}

bool Engine::Start() {
    Timer timer = Timer();

    LOG("Engine::Start");

    bool result = true;
    for (const auto& module : moduleList) {
        result = module.get()->Start();
        if (!result) {
            break;
        }
    }

    LOG("Timer App Start(): %f", timer.ReadMSec());

    return result;
}

bool Engine::Update() {
    ZoneScoped;

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

    FrameMark;

    return ret;
}

bool Engine::CleanUp() {
    Timer timer = Timer();

    LOG("Engine::CleanUp");

    bool result = true;
    for (const auto& module : moduleList) {
        result = module.get()->CleanUp();
        if (!result) {
            break;
        }
    }

    LOG("Timer App CleanUp(): %f", timer.ReadMSec());

    return result;
}

void Engine::PrepareUpdate()
{
    ZoneScoped;
    frameTime.Start();
}

void Engine::FinishUpdate()
{
    ZoneScoped;
    double currentDt = frameTime.ReadMs();
    if (maxFrameDuration > 0 && currentDt < maxFrameDuration) {
        int delay = (int)(maxFrameDuration - currentDt);

        PerfTimer delayTimer = PerfTimer();
        SDL_Delay(delay);
        // LOG("We waited for %I32u ms and got back in %f ms",delay,delayTimer.ReadMs());
    }

    frameCount++;
    secondsSinceStartup = startupTime.ReadSec();
    dt = (float)frameTime.ReadMs();
    lastSecFrameCount++;

    if (lastSecFrameTime.ReadMs() > 1000) {
        lastSecFrameTime.Start();
        averageFps = (averageFps + lastSecFrameCount) / 2;
        framesPerSecond = lastSecFrameCount;
        lastSecFrameCount = 0;
    }

    std::stringstream ss;
    ss << scene.get()->GetTilePosDebug()
        << gameTitle
        << ": Av.FPS: " << std::fixed
        << std::setprecision(2) << averageFps
        << " Last sec frames: " << framesPerSecond
        << " Last dt: " << std::fixed << std::setprecision(3) << dt
        << " Time since startup: " << secondsSinceStartup
        << " Frame Count: " << frameCount;

    std::string titleStr = ss.str();

    window.get()->SetTitle(titleStr.c_str());
}

bool Engine::PreUpdate()
{
    ZoneScoped;
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
    ZoneScoped;
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

bool Engine::LoadConfig()
{
    pugi::xml_parse_result result = configFile.load_file("config.xml");
    if (result)
    {
        LOG("config.xml parsed without errors");
        return true;
    }
    else
    {
        LOG("Error loading config.xml: %s", result.description());
        return false;
    }
}
