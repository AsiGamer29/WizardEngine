#define GLM_ENABLE_EXPERIMENTAL

#include "ModuleScene.h"
#include "Application.h"
#include "GameObject.h"
#include "BaseComponent.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "Texture.h"
#include "OpenGL.h"
#include "MeshImporter.h"
#include "TextureImporter.h"
#include "ModelImporter.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <glm/gtx/matrix_decompose.hpp>

#include <nlohmann/json.hpp>
#include <fstream>
#include <map>



ModuleScene::ModuleScene()
    : root(nullptr), selectedGameObject(nullptr)
{
    name = "ModuleScene";
}

ModuleScene::~ModuleScene()
{
    CleanUp();
}

bool ModuleScene::Start()
{
    std::cout << "[ModuleScene] Initializing..." << std::endl;

    // Crear Scene Root
    root = new GameObject("Scene Root");

    ComponentTransform* rootTransform = static_cast<ComponentTransform*>(
        root->CreateComponent(ComponentType::TRANSFORM)
        );

    if (rootTransform) {
        rootTransform->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        rootTransform->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
        rootTransform->SetRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    }

    allGameObjects.push_back(root);
    std::cout << "[ModuleScene] Root GameObject created with Transform" << std::endl;

    assetManager = new WizardEngine::AssetManager();
    if (!assetManager->Initialize()) {
        std::cerr << "[ModuleScene] Failed to initialize AssetManager" << std::endl;
        return false;
    }

    return true;
}

bool ModuleScene::PreUpdate()
{
    return true;
}

bool ModuleScene::Update()
{
    // Actualizar todos los GameObjects
    if (root)
    {
        root->Update();
    }

    return true;
}

bool ModuleScene::PostUpdate()
{
    return true;
}

void ModuleScene::RenderScene()
{
    auto& app = Application::GetInstance();

    if (!app.opengl)
        return;

    app.opengl->DrawGrid();

    if (root)
    {
        app.opengl->DrawGameObjectsWithAABB(root);
    }
}

bool ModuleScene::CleanUp()
{
    if (assetManager) {
        delete assetManager;
        assetManager = nullptr;
    }

    std::cout << "[ModuleScene] Cleaning up..." << std::endl;
    ClearScene();

    return true;
}

GameObject* ModuleScene::CreateGameObject(const char* name, GameObject* parent)
{
    GameObject* go = new GameObject(name);

    ComponentTransform* transform = static_cast<ComponentTransform*>(
        go->CreateComponent(ComponentType::TRANSFORM)
        );

    if (transform) {
        transform->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        transform->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
        transform->SetRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    }

    if (parent) {
        go->SetParent(parent);
    }

    allGameObjects.push_back(go);
    return go;
}

void ModuleScene::DestroyGameObject(GameObject* gameObject)
{
    if (!gameObject || gameObject == root)
        return;

    // Eliminar de la lista global
    auto it = std::find(allGameObjects.begin(), allGameObjects.end(), gameObject);
    if (it != allGameObjects.end())
    {
        allGameObjects.erase(it);
    }

    // Eliminar de su padre
    if (gameObject->GetParent())
    {
        gameObject->GetParent()->RemoveChild(gameObject);
    }

    // Eliminar el GameObject (su destructor eliminará sus hijos)
    delete gameObject;
}

void ModuleScene::ClearScene()
{
    SetSelectedGameObject(nullptr);

    // Crear una copia del vector para evitar problemas de iteracion
    std::vector<GameObject*> objectsToDelete = allGameObjects;
    allGameObjects.clear();
    root = nullptr;

    // Primero: romper todas las relaciones
    for (GameObject* go : objectsToDelete)
    {
        if (go)
        {
            go->ClearHierarchyReferences();
        }
    }

    // Segundo: eliminar todos los objetos
    for (GameObject* go : objectsToDelete)
    {
        if (go)
        {
            delete go;
        }
    }

    ModuleEditor::PushEngineLog("Scene cleared");
}

void ModuleScene::LoadModel(const char* path)
{
    std::cout << "[ModuleScene] Loading model: " << path << std::endl;

    if (!root)
    {
        root = new GameObject("Scene Root");

        ComponentTransform* rootTransform = static_cast<ComponentTransform*>(
            root->CreateComponent(ComponentType::TRANSFORM)
            );

        if (rootTransform) {
            rootTransform->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
            rootTransform->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
            rootTransform->SetRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        }

        allGameObjects.push_back(root);
    }

    Assimp::Importer* importer = new Assimp::Importer();

    const aiScene* scene = importer->ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "[ModuleScene] ERROR loading model: " << importer->GetErrorString() << std::endl;
        delete importer;
        return;
    }

    std::string pathStr(path);
    std::string basePath = pathStr.substr(0, pathStr.find_last_of("/\\"));

    LoadFromAssimp(scene, scene->mRootNode, root, basePath);

    std::cout << "[ModuleScene] Model hierarchy loaded" << std::endl;

    delete importer;
}

void ModuleScene::LoadFromAssimp(const aiScene* scene, const aiNode* node, GameObject* parent, const std::string& basePath)
{
    GameObject* gameObject = CreateGameObject(node->mName.C_Str(), parent);

    ComponentTransform* transform = (ComponentTransform*)gameObject->CreateComponent(ComponentType::TRANSFORM);

    aiVector3D translation, scaling;
    aiQuaternion rotation;
    node->mTransformation.Decompose(scaling, rotation, translation);

    glm::vec3 pos(translation.x, translation.y, translation.z);
    glm::vec3 scale(scaling.x, scaling.y, scaling.z);
    glm::quat rot(rotation.w, rotation.x, rotation.y, rotation.z);

    transform->SetPosition(pos);
    transform->SetScale(scale);
    transform->SetRotation(rot);

    std::cout << "[ModuleScene] Created GameObject: " << node->mName.C_Str()
        << " with transform" << std::endl;

    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        unsigned int meshIndex = node->mMeshes[i];
        const aiMesh* mesh = scene->mMeshes[meshIndex];

        std::cout << "[ModuleScene] - Processing mesh " << i << ": " << mesh->mName.C_Str() << std::endl;

        GameObject* meshGameObject = gameObject;
        if (i > 0)
        {
            std::string meshName = std::string(node->mName.C_Str()) + "_mesh_" + std::to_string(i);
            meshGameObject = CreateGameObject(meshName.c_str(), gameObject);

            ComponentTransform* subTransform = (ComponentTransform*)meshGameObject->CreateComponent(ComponentType::TRANSFORM);
            subTransform->SetPosition(glm::vec3(0.0f));
            subTransform->SetScale(glm::vec3(1.0f));
            subTransform->SetRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        }

        ComponentMesh* compMesh = (ComponentMesh*)meshGameObject->CreateComponent(ComponentType::MESH);
        compMesh->LoadMesh(mesh);

        std::cout << "[ModuleScene]   - Loaded mesh with "
            << mesh->mNumVertices << " vertices and "
            << mesh->mNumFaces << " faces" << std::endl;

        ComponentMaterial* compMaterial = (ComponentMaterial*)meshGameObject->CreateComponent(ComponentType::MATERIAL);

        if (mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < scene->mNumMaterials)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            unsigned int numTextures = material->GetTextureCount(aiTextureType_DIFFUSE);

            if (numTextures > 0)
            {
                aiString texPath;
                if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
                {
                    std::string fullPath = basePath + "/" + std::string(texPath.C_Str());
                    std::cout << "[ModuleScene]   - Loading texture: " << fullPath << std::endl;
                    compMaterial->LoadTexture(fullPath.c_str());

                    std::string fileName = texPath.C_Str();
                    size_t lastSlash = fileName.find_last_of("/\\");
                    if (lastSlash != std::string::npos)
                    {
                        fileName = fileName.substr(lastSlash + 1);
                        std::string altPath = basePath + "/" + fileName;
                        std::cout << "[ModuleScene]   - Alternative path: " << altPath << std::endl;
                    }
                }
            }
            else
            {
                std::cout << "[ModuleScene]   - No texture found, using checkerboard" << std::endl;
            }
        }
        else
        {
            std::cout << "[ModuleScene]   - Invalid material index, using checkerboard" << std::endl;
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        LoadFromAssimp(scene, node->mChildren[i], gameObject, basePath);
    }

    gameObject->UpdateAABB();
}

void ModuleScene::UpdateAllAABBs()
{
    for (GameObject* go : allGameObjects)
    {
        if (go)
        {
            go->UpdateAABB();
        }
    }
}

GameObject* ModuleScene::PerformRaycast(const Ray& ray)
{
    std::vector<RayHit> candidates;

    if (root)
        CollectRaycastCandidates(root, ray, candidates);

    if (candidates.empty())
        return nullptr;

    std::sort(candidates.begin(), candidates.end(),
        [](const RayHit& a, const RayHit& b) {
            return a.distance < b.distance;
        });

    return candidates[0].gameObject;
}

void ModuleScene::CollectRaycastCandidates(GameObject* go, const Ray& ray, std::vector<RayHit>& candidates)
{
    if (!go || !go->IsActive())
        return;

    // CRITICO: Solo hacer raycast si tiene AABB valido
    if (go->HasAABB())
    {
        RayHit hit;
        if (go->IntersectRay(ray, hit))
        {
            candidates.push_back(hit);
        }
    }

    // Recursivamente procesar hijos
    for (GameObject* child : go->GetChildren())
    {
        CollectRaycastCandidates(child, ray, candidates);
    }
}

bool ModuleScene::SaveScene(const std::string& filepath)
{
    nlohmann::json sceneJson;
    nlohmann::json gameObjectsArray = nlohmann::json::array();

    // Serializar TODOS los GameObjects (incluyendo el Root)
    for (GameObject* go : allGameObjects)
    {
        if (go)
        {
            gameObjectsArray.push_back(go->Serialize());
        }
    }

    sceneJson["GameObjects"] = gameObjectsArray;

    // Guardar a archivo
    std::ofstream file(filepath);
    if (!file.is_open())
    {
        ModuleEditor::PushEnginePrintf("ERROR: Cannot create file: %s", filepath.c_str());
        return false;
    }

    try
    {
        file << sceneJson.dump(4);  // Pretty print con indentacion
    }
    catch (const std::exception& e)
    {
        ModuleEditor::PushEnginePrintf("ERROR: Failed to write JSON: %s", e.what());
        file.close();
        return false;
    }

    file.close();
    ModuleEditor::PushEnginePrintf("Scene saved: %s (%zu GameObjects)",
        filepath.c_str(), allGameObjects.size());

    return true;
}

bool ModuleScene::LoadScene(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        ModuleEditor::PushEnginePrintf("ERROR: Cannot open scene file: %s", filepath.c_str());
        return false;
    }

    nlohmann::json sceneJson;
    try
    {
        file >> sceneJson;
    }
    catch (const std::exception& e)
    {
        ModuleEditor::PushEnginePrintf("ERROR: Failed to parse JSON: %s", e.what());
        file.close();
        return false;
    }
    file.close();

    // PASO 0: Limpiar escena actual (incluyendo el Root)
    ClearScene();

    // NO crear un nuevo Root aqui

    if (!sceneJson.contains("GameObjects") || !sceneJson["GameObjects"].is_array())
    {
        ModuleEditor::PushEngineLog("ERROR: Scene file has no GameObjects array");
        return false;
    }

    const auto& gameObjectsArray = sceneJson["GameObjects"];

    // PASO 1: Crear todos los GameObjects y mapear UIDs
    std::map<uint32_t, GameObject*> uidToGameObject;
    std::map<GameObject*, uint32_t> gameObjectToParentUID;

    for (const auto& goJson : gameObjectsArray)
    {
        if (!goJson.contains("UID") || !goJson.contains("Name"))
        {
            ModuleEditor::PushEngineLog("WARNING: GameObject missing UID or Name, skipping");
            continue;
        }

        uint32_t uid = goJson["UID"].get<uint32_t>();
        std::string name = goJson["Name"].get<std::string>();

        // Crear GameObject SIN padre (temporalmente)
        GameObject* newGO = new GameObject(name.c_str(), nullptr);
        newGO->SetUUID(UUID(uid));

        // Deserializar componentes y propiedades
        newGO->Deserialize(goJson);

        // Guardar en el mapa
        uidToGameObject[uid] = newGO;
        allGameObjects.push_back(newGO);

        // Guardar UID del padre para la segunda pasada
        if (goJson.contains("ParentUID"))
        {
            uint32_t parentUID = goJson["ParentUID"].get<uint32_t>();
            if (parentUID != 0)
            {
                gameObjectToParentUID[newGO] = parentUID;
            }
        }
    }

    // PASO 2: Restaurar jerarquia padre-hijo
    GameObject* newRoot = nullptr;

    for (const auto& pair : gameObjectToParentUID)
    {
        GameObject* child = pair.first;
        uint32_t parentUID = pair.second;

        auto it = uidToGameObject.find(parentUID);
        if (it != uidToGameObject.end())
        {
            GameObject* parent = it->second;
            child->SetParent(parent);
        }
        else
        {
            ModuleEditor::PushEnginePrintf("WARNING: Parent UID %u not found for %s",
                parentUID, child->GetName());
        }
    }

    // PASO 3: Encontrar el objeto raiz (el que no tiene padre)
    for (GameObject* go : allGameObjects)
    {
        if (go->GetParent() == nullptr)
        {
            newRoot = go;
            break;
        }
    }

    // Si no hay root en la escena, crear uno vacio
    if (!newRoot)
    {
        ModuleEditor::PushEngineLog("WARNING: No root found in scene, creating empty root");
        newRoot = new GameObject("Scene Root", nullptr);
        allGameObjects.push_back(newRoot);
    }

    root = newRoot;

    UpdateAllAABBs();

    ModuleEditor::PushEnginePrintf("Scene loaded: %s (%zu GameObjects)",
        filepath.c_str(), allGameObjects.size());

    return true;
}

GameObject* ModuleScene::ImportModelFromAssets(const std::string& assetPath) {
    if (!assetManager) {
        std::cerr << "[ModuleScene] AssetManager not initialized" << std::endl;
        return nullptr;
    }

    std::cout << "[ModuleScene] Importing model from Assets..." << std::endl;

    // Procesar archivo (esto crea los .wzm, .wzt, .wzd)
    if (!assetManager->ProcessAssetFile(assetPath)) {
        std::cerr << "[ModuleScene] Failed to process asset: " << assetPath << std::endl;
        return nullptr;
    }

    // Obtener ruta del archivo .wzd generado
    std::string libraryPath = assetManager->GetLibraryPath(assetPath, "wzd");
    if (libraryPath.empty()) {
        std::cerr << "[ModuleScene] Failed to get library path" << std::endl;
        return nullptr;
    }

    // Cargar desde Library
    return LoadModelFromLibrary(libraryPath);
}

GameObject* ModuleScene::LoadModelFromLibrary(const std::string& libraryPath) {
    std::cout << "[ModuleScene] Loading model from Library..." << std::endl;

    // Cargar metadata del modelo
    WizardEngine::WizardModelData modelData;
    if (!WizardEngine::ModelImporter::Load(libraryPath, modelData)) {
        std::cerr << "[ModuleScene] Failed to load model: " << libraryPath << std::endl;
        return nullptr;
    }

    // Crear GameObject root para el modelo
    std::string modelName = std::filesystem::path(libraryPath).parent_path().filename().string();
    GameObject* rootObject = CreateGameObject(modelName.c_str(), GetRoot());

    // Crear nodos de jerarquia
    std::vector<GameObject*> nodeObjects;
    nodeObjects.resize(modelData.nodes.size());

    for (size_t i = 0; i < modelData.nodes.size(); i++) {
        const auto& nodeData = modelData.nodes[i];

        GameObject* parent = (nodeData.parentIndex >= 0)
            ? nodeObjects[nodeData.parentIndex]
            : rootObject;

        GameObject* nodeObject = CreateGameObject(nodeData.name.c_str(), parent);
        nodeObjects[i] = nodeObject;

        // Aplicar transformacion
        ComponentTransform* transform = nodeObject->GetComponent<ComponentTransform>();
        if (transform) {
            // Descomponer matriz
            glm::vec3 scale, translation, skew;
            glm::vec4 perspective;
            glm::quat rotation;
            glm::decompose(nodeData.transformation, scale, rotation, translation, skew, perspective);

            transform->SetPosition(translation);
            transform->SetRotation(rotation);
            transform->SetScale(scale);
        }

        // Cargar meshes asociados a este nodo
        for (int meshIndex : nodeData.meshIndices) {
            if (meshIndex < 0 || meshIndex >= modelData.meshes.size()) continue;

            const auto& meshRef = modelData.meshes[meshIndex];

            // Cargar mesh desde .wzm
            WizardEngine::WizardMeshData meshData;
            if (!WizardEngine::MeshImporter::Load(meshRef.meshFilepath, meshData)) {
                std::cerr << "[ModuleScene] Failed to load mesh: " << meshRef.meshFilepath << std::endl;
                continue;
            }

            // Crear componente mesh
            ComponentMesh* meshComp = static_cast<ComponentMesh*>(
                nodeObject->CreateComponent(ComponentType::MESH)
                );

            if (meshComp) {
                // Convertir WizardMeshData a formato del engine
                meshComp->LoadFromWizardFormat(meshData);
                nodeObject->UpdateAABB();
            }

            // Cargar material
            if (meshRef.materialIndex < modelData.materials.size()) {
                const auto& matData = modelData.materials[meshRef.materialIndex];

                ComponentMaterial* matComp = static_cast<ComponentMaterial*>(
                    nodeObject->CreateComponent(ComponentType::MATERIAL)
                    );

                if (matComp && !matData.diffuseTexture.empty()) {
                    // Cargar textura desde .wzt
                    WizardEngine::WizardTextureData texData;
                    if (WizardEngine::TextureImporter::Load(matData.diffuseTexture, texData)) {
                        // Crear textura OpenGL
                        GLuint texID;
                        glGenTextures(1, &texID);
                        glBindTexture(GL_TEXTURE_2D, texID);

                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                            texData.width, texData.height,
                            0, GL_RGBA, GL_UNSIGNED_BYTE, texData.data.data());
                        glGenerateMipmap(GL_TEXTURE_2D);

                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                        matComp->SetTexture(texID, matData.diffuseTexture.c_str(), texData.channels);
                    }
                }
            }
        }
    }

    UpdateAllAABBs();

    std::cout << "[ModuleScene] Model loaded successfully!" << std::endl;
    return rootObject;
}