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
    if (!gameObject)
    {
        return;
    }

    std::cout << "[ModuleScene] Destroying GameObject: " << gameObject->GetName() << std::endl;

    // PRIMERO: Destruir todos los hijos recursivamente
    std::vector<GameObject*> children = gameObject->GetChildren();
    for (GameObject* child : children)
    {
        if (child)
        {
            DestroyGameObject(child);
        }
    }

    // Si este objeto es el seleccionado, deseleccionar
    if (selectedGameObject == gameObject)
    {
        selectedGameObject = nullptr;
    }

    // Remover de la lista de todos los objetos
    auto it = std::find(allGameObjects.begin(), allGameObjects.end(), gameObject);
    if (it != allGameObjects.end())
    {
        allGameObjects.erase(it);
    }

    // Remover del padre (si tiene)
    if (gameObject->GetParent())
    {
        gameObject->GetParent()->RemoveChild(gameObject);
    }

    // FINALMENTE: Eliminar el objeto
    delete gameObject;

    std::cout << "[ModuleScene] GameObject destroyed successfully" << std::endl;
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

    // El Transform ya existe, solo lo obtenemos
    ComponentTransform* transform = gameObject->GetComponent<ComponentTransform>();

    if (transform)
    {
        // Convertir la matriz de Assimp a GLM
        aiMatrix4x4 assimpMatrix = node->mTransformation;

        glm::mat4 localMatrix(
            assimpMatrix.a1, assimpMatrix.b1, assimpMatrix.c1, assimpMatrix.d1,
            assimpMatrix.a2, assimpMatrix.b2, assimpMatrix.c2, assimpMatrix.d2,
            assimpMatrix.a3, assimpMatrix.b3, assimpMatrix.c3, assimpMatrix.d3,
            assimpMatrix.a4, assimpMatrix.b4, assimpMatrix.c4, assimpMatrix.d4
        );

        // Descomponer usando GLM (mas preciso)
        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;

        glm::decompose(localMatrix, scale, rotation, translation, skew, perspective);

        // Aplicar transformacion LOCAL (no global)
        transform->SetPosition(translation);
        transform->SetRotation(rotation);
        transform->SetScale(scale);

        std::cout << "[ModuleScene] Node: " << node->mName.C_Str()
            << " | Pos: (" << translation.x << ", " << translation.y << ", " << translation.z << ")"
            << " | Scale: (" << scale.x << ", " << scale.y << ", " << scale.z << ")" << std::endl;
    }

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

            // Los submeshes mantienen transform identidad (ya estan en el espacio del nodo padre)
            ComponentTransform* subTransform = meshGameObject->GetComponent<ComponentTransform>();
            if (subTransform)
            {
                subTransform->SetPosition(glm::vec3(0.0f));
                subTransform->SetScale(glm::vec3(1.0f));
                subTransform->SetRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            }
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

    // Procesar hijos DESPUES de configurar este nodo
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
    std::string libraryPath = assetManager->GetLibraryPath(assetPath);
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

GameObject* ModuleScene::DuplicateGameObject(GameObject* original)
{
    if (!original)
    {
        std::cerr << "[ModuleScene] Cannot duplicate null GameObject" << std::endl;
        return nullptr;
    }

    std::cout << "[ModuleScene] === Starting duplication of: " << original->GetName() << " ===" << std::endl;

    // Crear nuevo GameObject con nombre duplicado
    std::string newName = std::string(original->GetName()) + "_Copy";
    GameObject* duplicate = CreateGameObject(newName.c_str(), original->GetParent());

    if (!duplicate)
    {
        std::cerr << "[ModuleScene] FAILED to create duplicate GameObject" << std::endl;
        return nullptr;
    }

    // ===== PASO 1: Copiar Transform =====
    ComponentTransform* originalTransform = original->GetComponent<ComponentTransform>();
    ComponentTransform* duplicateTransform = duplicate->GetComponent<ComponentTransform>();

    if (originalTransform && duplicateTransform)
    {
        duplicateTransform->SetPosition(originalTransform->GetPosition());
        duplicateTransform->SetRotation(originalTransform->GetRotation());
        duplicateTransform->SetScale(originalTransform->GetScale());
        std::cout << "[ModuleScene]   - Transform copied" << std::endl;
    }

    // ===== PASO 2: Copiar Mesh =====
    ComponentMesh* originalMesh = original->GetComponent<ComponentMesh>();
    if (originalMesh && originalMesh->GetVertexCount() > 0)
    {
        std::cout << "[ModuleScene]   - Copying Mesh (vertices: " << originalMesh->GetVertexCount() << ")" << std::endl;

        ComponentMesh* duplicateMesh = duplicate->GetComponent<ComponentMesh>();
        if (!duplicateMesh)
        {
            duplicateMesh = static_cast<ComponentMesh*>(
                duplicate->CreateComponent(ComponentType::MESH)
                );
        }

        if (duplicateMesh)
        {
            try
            {
                nlohmann::json meshJson = originalMesh->SerializeMesh();
                duplicateMesh->DeserializeMesh(meshJson);
                std::cout << "[ModuleScene]   - Mesh copied successfully" << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cerr << "[ModuleScene]   - ERROR copying mesh: " << e.what() << std::endl;
            }
        }
    }
    else
    {
        std::cout << "[ModuleScene]   - No mesh to copy" << std::endl;
    }

    // ===== PASO 3: Copiar Material =====
    ComponentMaterial* originalMat = original->GetComponent<ComponentMaterial>();
    if (originalMat)
    {
        std::cout << "[ModuleScene]   - Copying Material" << std::endl;

        ComponentMaterial* duplicateMat = duplicate->GetComponent<ComponentMaterial>();
        if (!duplicateMat)
        {
            duplicateMat = static_cast<ComponentMaterial*>(
                duplicate->CreateComponent(ComponentType::MATERIAL)
                );
        }

        if (duplicateMat)
        {
            const char* texPath = originalMat->GetTexturePath();

            if (texPath && texPath[0] != '\0')
            {
                if (strcmp(texPath, "checkerboard_default") != 0)
                {
                    std::cout << "[ModuleScene]   - Loading texture: " << texPath << std::endl;
                    duplicateMat->LoadTexture(texPath);
                }
                else
                {
                    std::cout << "[ModuleScene]   - Applying checkerboard" << std::endl;
                    GLuint checkerTex = Texture::CreateCheckerboardTexture(512, 512, 32);
                    duplicateMat->SetTexture(checkerTex, "checkerboard_default", 3);
                }
            }
            else
            {
                std::cout << "[ModuleScene]   - No texture path, applying checkerboard" << std::endl;
                GLuint checkerTex = Texture::CreateCheckerboardTexture(512, 512, 32);
                duplicateMat->SetTexture(checkerTex, "checkerboard_default", 3);
            }

            // Copiar propiedades
            duplicateMat->SetAlphaMode(originalMat->GetAlphaMode());
            duplicateMat->SetAlphaCutoff(originalMat->GetAlphaCutoff());
            duplicateMat->SetBlendMode(originalMat->GetBlendMode());

            std::cout << "[ModuleScene]   - Material copied successfully" << std::endl;
        }
    }
    else
    {
        std::cout << "[ModuleScene]   - No material to copy" << std::endl;
    }

    // ===== PASO 4: Actualizar AABB =====
    duplicate->UpdateAABB();

    // ===== PASO 5: Duplicar hijos recursivamente =====
    const std::vector<GameObject*>& children = original->GetChildren();

    if (children.size() > 0)
    {
        std::cout << "[ModuleScene]   - Duplicating " << children.size() << " children..." << std::endl;

        int successCount = 0;
        for (size_t i = 0; i < children.size(); i++)
        {
            GameObject* child = children[i];
            if (!child)
            {
                std::cerr << "[ModuleScene]     - Child " << i << " is null, skipping" << std::endl;
                continue;
            }

            std::cout << "[ModuleScene]     - Duplicating child " << (i + 1) << "/" << children.size()
                << ": " << child->GetName() << std::endl;

            GameObject* duplicatedChild = DuplicateGameObject(child);

            if (duplicatedChild)
            {
                // Obtener transforms ANTES de cambiar el padre
                ComponentTransform* childOriginalTransform = child->GetComponent<ComponentTransform>();

                if (childOriginalTransform)
                {
                    glm::vec3 localPos = childOriginalTransform->GetPosition();
                    glm::quat localRot = childOriginalTransform->GetRotation();
                    glm::vec3 localScale = childOriginalTransform->GetScale();

                    // Cambiar padre
                    duplicatedChild->SetParent(duplicate);

                    // Restaurar transform local
                    ComponentTransform* childDuplicateTransform = duplicatedChild->GetComponent<ComponentTransform>();
                    if (childDuplicateTransform)
                    {
                        childDuplicateTransform->SetPosition(localPos);
                        childDuplicateTransform->SetRotation(localRot);
                        childDuplicateTransform->SetScale(localScale);
                    }
                }
                else
                {
                    duplicatedChild->SetParent(duplicate);
                }

                successCount++;
                std::cout << "[ModuleScene]     - Child duplicated successfully" << std::endl;
            }
            else
            {
                std::cerr << "[ModuleScene]     - FAILED to duplicate child: " << child->GetName() << std::endl;
            }
        }

        std::cout << "[ModuleScene]   - Children duplication complete: "
            << successCount << "/" << children.size() << " successful" << std::endl;
    }

    std::cout << "[ModuleScene] === Duplication complete: " << duplicate->GetName() << " ===" << std::endl;
    return duplicate;
}

GameObject* ModuleScene::LoadModelFromAssetPath(const std::string& assetPath)
{
    std::cout << "\n==================================================" << std::endl;
    std::cout << "[ModuleScene] LoadModelFromAssetPath: " << assetPath << std::endl;

    if (!assetManager || !root)
    {
        std::cerr << "[ModuleScene] ERROR: AssetManager or root not initialized" << std::endl;
        return nullptr;
    }

    // Normalizar ruta
    std::string normalizedPath = assetPath;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

    if (!std::filesystem::exists(normalizedPath))
    {
        std::cerr << "[ModuleScene] ERROR: Asset not found: " << normalizedPath << std::endl;
        return nullptr;
    }

    // Procesar asset
    if (!assetManager->ProcessAssetFile(normalizedPath))
    {
        std::cerr << "[ModuleScene] ERROR: Failed to process asset" << std::endl;
        return nullptr;
    }

    // Obtener ruta .wzd
    std::string libraryPath = assetManager->GetLibraryPath(normalizedPath);
    if (libraryPath.empty() || !std::filesystem::exists(libraryPath))
    {
        std::cerr << "[ModuleScene] ERROR: Library file not found" << std::endl;
        return nullptr;
    }

    // Cargar modelo
    WizardEngine::WizardModelData modelData;
    if (!WizardEngine::ModelImporter::Load(libraryPath, modelData))
    {
        std::cerr << "[ModuleScene] ERROR: Failed to load WZD" << std::endl;
        return nullptr;
    }

    std::cout << "[ModuleScene] Model loaded: " << modelData.meshes.size() << " meshes" << std::endl;

    // Crear root
    std::string modelName = std::filesystem::path(normalizedPath).stem().string();
    GameObject* modelRoot = CreateGameObject(modelName.c_str(), root);

    // Aplicar import settings
    WizardEngine::AssetMetaData* metaData = assetManager->GetMetaData(normalizedPath);
    ComponentTransform* rootTransform = modelRoot->GetComponent<ComponentTransform>();

    if (rootTransform)
    {
        if (metaData)
        {
            rootTransform->SetScale(metaData->importScale);
            rootTransform->SetRotation(glm::quat(glm::radians(metaData->importRotation)));
        }
        rootTransform->SetPosition(glm::vec3(0.0f));
    }

    // Cargar meshes
    int meshCount = 0;
    for (size_t i = 0; i < modelData.meshes.size(); i++)
    {
        const auto& meshRef = modelData.meshes[i];

        // Cargar mesh data
        WizardEngine::WizardMeshData meshData;
        if (!WizardEngine::MeshImporter::Load(meshRef.meshFilepath, meshData))
        {
            std::cerr << "[ModuleScene] WARNING: Failed to load: " << meshRef.meshFilepath << std::endl;
            continue;
        }

        // Crear GameObject
        std::string meshName = modelName + "_mesh_" + std::to_string(i);
        GameObject* meshObj = CreateGameObject(meshName.c_str(), modelRoot);

        // Crear ComponentMesh
        ComponentMesh* meshComp = static_cast<ComponentMesh*>(
            meshObj->CreateComponent(ComponentType::MESH)
            );

        if (meshComp)
        {
            // Convertir a MeshGeometry
            MeshGeometry geom;
            geom.vertices.reserve(meshData.vertices.size());

            for (const auto& v : meshData.vertices)
            {
                GeomVertex gv;
                gv.Position = v.position;
                gv.Normal = v.normal;
                gv.TexCoords = v.texCoords;
                geom.vertices.push_back(gv);
            }

            geom.indices = meshData.indices;

            // CRÍTICO: Cargar geometría Y establecer path
            meshComp->LoadFromGeometry(&geom);
            meshComp->SetSourceAssetPath(meshRef.meshFilepath);  // ¡IMPORTANTE!

            std::cout << "[ModuleScene]   Mesh " << i << ": " << meshData.vertices.size()
                << " verts, path: " << meshRef.meshFilepath << std::endl;

            meshCount++;
        }

        // Cargar material
        ComponentMaterial* matComp = static_cast<ComponentMaterial*>(
            meshObj->CreateComponent(ComponentType::MATERIAL)
            );

        if (matComp && meshRef.materialIndex < modelData.materials.size())
        {
            const auto& matData = modelData.materials[meshRef.materialIndex];

            if (!matData.diffuseTexture.empty())
            {
                WizardEngine::WizardTextureData texData;
                if (WizardEngine::TextureImporter::Load(matData.diffuseTexture, texData))
                {
                    GLuint texID;
                    glGenTextures(1, &texID);
                    glBindTexture(GL_TEXTURE_2D, texID);

                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                        texData.width, texData.height, 0,
                        GL_RGBA, GL_UNSIGNED_BYTE, texData.data.data());

                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                    matComp->SetTexture(texID, matData.diffuseTexture.c_str(), texData.channels);
                }
                else
                {
                    GLuint checkerTex = Texture::CreateCheckerboardTexture(512, 512, 32);
                    matComp->SetTexture(checkerTex, "checkerboard_default", 3);
                }
            }
            else
            {
                GLuint checkerTex = Texture::CreateCheckerboardTexture(512, 512, 32);
                matComp->SetTexture(checkerTex, "checkerboard_default", 3);
            }
        }

        meshObj->UpdateAABB();
    }

    UpdateAllAABBs();

    std::cout << "[ModuleScene] SUCCESS: " << meshCount << "/" << modelData.meshes.size()
        << " meshes loaded\n==================================================" << std::endl;

    return modelRoot;
}