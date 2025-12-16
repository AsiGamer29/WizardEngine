#include "Application.h"
#include "Texture.h"
#include "ModuleScene.h"
#include "ComponentMaterial.h"
#include "Module.h"
#include <iostream>

Application::Application() : isRunning(true)
{
    std::cout << "Application Constructor" << std::endl;

    // ORDEN CORRECTO: ModuleScene PRIMERO
    moduleScene = std::make_shared<ModuleScene>();
    window = std::make_shared<Window>();
    input = std::make_shared<Input>();
    opengl = std::make_shared<OpenGL>();
    editor = std::make_shared<ModuleEditor>();
    camera = std::make_shared<Camera>(
        glm::vec3(0.0f, 0.0f, 5.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        -90.0f, 0.0f
    );
    assetManager = std::make_shared<WizardEngine::AssetManager>();

    // ORDEN DE INICIALIZACIÓN (importante):
    // 1. ModuleScene (crea el root)
    // 2. Window
    // 3. Input
    // 4. OpenGL (necesita ModuleScene ya inicializado)
    // 5. Editor (último)
    AddModule(std::static_pointer_cast<Module>(moduleScene));
    AddModule(std::static_pointer_cast<Module>(window));
    AddModule(std::static_pointer_cast<Module>(input));
    AddModule(std::static_pointer_cast<Module>(opengl));
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
    bool ret = true;

    for (const auto& modulePtr : moduleList)
    {
        Module* module = modulePtr.get();
        if (module)
        {
            ret = module->Start();
            if (!ret)
            {
                std::cerr << "Module " << module->name << " failed to start!" << std::endl;
                break;
            }
        }
    }

    // AHORA cargar el modelo (después de que OpenGL esté inicializado)
    if (moduleScene && ret)
    {
        std::string modelPath = "../Assets/Models/Street/scene.DAE";

        std::cout << "[Application] Loading street model..." << std::endl;

        if (std::filesystem::exists(modelPath))
        {
            moduleScene->LoadModel(modelPath.c_str());

            std::cout << "[Application] Street model loaded and textured!" << std::endl;
        }
        else
        {
            std::cerr << "[Application] WARNING: street2.FBX not found at: " << modelPath << std::endl;
        }
    }

    return ret;
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
