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
	moduleResources = std::make_shared<WizardEngine::ModuleResources>();

    AddModule(std::static_pointer_cast<Module>(moduleResources));
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
        std::string modelPath = "../Assets/Models/Street/street2.FBX";

        std::cout << "[Application] Loading street model..." << std::endl;

        if (std::filesystem::exists(modelPath))
        {
            moduleScene->LoadModel(modelPath.c_str());

            // Asignar texturas
            AssignStreetTextures();

            std::cout << "[Application] Street model loaded and textured!" << std::endl;
        }
        else
        {
            std::cerr << "[Application] WARNING: street2.FBX not found at: " << modelPath << std::endl;
        }
    }

    return ret;
}

void Application::AssignStreetTextures()
{
    if (!moduleScene) return;

    std::cout << "[Application] Assigning textures to street model..." << std::endl;

    // Mapeo de nombres de GameObjects a texturas
    std::map<std::string, std::string> textureMap = {
        {"building 01", "../Assets/Models/Street/building 01_c.tga"},
        {"building_01", "../Assets/Models/Street/building 01_c.tga"},

        {"building 06", "../Assets/Models/Street/building 06_c.tga"},
        {"building_06", "../Assets/Models/Street/building 06_c.tga"},

        {"building 016", "../Assets/Models/Street/building 016_c.tga"},
        {"building_016", "../Assets/Models/Street/building 016_c.tga"},

        {"building 025", "../Assets/Models/Street/building 025_c.tga"},
        {"building_025", "../Assets/Models/Street/building 025_c.tga"},

        {"building03", "../Assets/Models/Street/building03_c.tga"},
        {"building_03", "../Assets/Models/Street/building03_c.tga"},

        {"building05", "../Assets/Models/Street/building05_c.tga"},
        {"building_05", "../Assets/Models/Street/building05_c.tga"},

        {"Building_V01", "../Assets/Models/Street/Building_V01_C.png"},
        {"building_v01", "../Assets/Models/Street/Building_V01_C.png"},

        {"Building_V02", "../Assets/Models/Street/Building_V02_C.png"},
        {"building_v02", "../Assets/Models/Street/Building_V02_C.png"}
    };

    const std::vector<GameObject*>& allObjects = moduleScene->GetAllGameObjects();
    int texturedCount = 0;

    for (GameObject* obj : allObjects)
    {
        if (!obj) continue;

        std::string objName = obj->GetName();
        std::string objNameLower = objName;

        std::transform(objNameLower.begin(), objNameLower.end(), objNameLower.begin(), ::tolower);

        for (const auto& pair : textureMap)
        {
            std::string keyLower = pair.first;
            std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), ::tolower);

            if (objNameLower.find(keyLower) != std::string::npos)
            {
                ComponentMaterial* mat = obj->GetComponent<ComponentMaterial>();

                if (!mat)
                {
                    mat = static_cast<ComponentMaterial*>(
                        obj->CreateComponent(ComponentType::MATERIAL)
                        );
                }

                if (mat)
                {
                    const std::string& texturePath = pair.second;

                    if (std::filesystem::exists(texturePath))
                    {
                        mat->LoadTexture(texturePath.c_str());
                        std::cout << "[Application]   - Assigned texture to '" << objName
                            << "': " << texturePath << std::endl;
                        texturedCount++;
                    }
                    else
                    {
                        std::cout << "[Application]   - Texture not found for '" << objName
                            << "': " << texturePath << std::endl;

                        GLuint checkerTex = Texture::CreateCheckerboardTexture(512, 512, 32);
                        mat->SetTexture(checkerTex, "checkerboard_default", 3);
                    }
                }

                break;
            }
        }
    }

    std::cout << "[Application] Textured " << texturedCount << " objects" << std::endl;
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

WizardEngine::AssetManager* Application::GetAssetManager() {
    if (moduleResources) {
        return moduleResources->GetAssetManager();
    }
    return nullptr;
}