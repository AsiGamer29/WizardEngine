#include "ModuleScene.h"
#include "GameObject.h"
#include "BaseComponent.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "Texture.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>


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

    // Crear GameObject raíz (root)
    root = new GameObject("Scene Root");
    allGameObjects.push_back(root);

    std::cout << "[ModuleScene] Root GameObject created" << std::endl;

    return true;
}

bool ModuleScene::PreUpdate()
{
    return true;
}

bool ModuleScene::Update()
{
    // Actualizar todos los GameObjects (esto llamará Update en sus componentes)
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

bool ModuleScene::CleanUp()
{
    std::cout << "[ModuleScene] Cleaning up..." << std::endl;

    ClearScene();

    return true;
}

GameObject* ModuleScene::CreateGameObject(const char* name, GameObject* parent)
{
    GameObject* newGO = new GameObject(name);

    // Si no se especifica padre, usar el root
    if (parent == nullptr)
        parent = root;

    // Añadir a la jerarquía
    if (parent)
    {
        parent->AddChild(newGO);
    }

    // Añadir a la lista global
    allGameObjects.push_back(newGO);

    std::cout << "[ModuleScene] Created GameObject: " << name << std::endl;

    return newGO;
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

    // Eliminar recursivamente todos los hijos
    RecursiveDelete(gameObject);
}

void ModuleScene::RecursiveDelete(GameObject* go)
{
    if (!go) return;

    // Eliminar todos los hijos primero
    std::vector<GameObject*> children = go->GetChildren();
    for (GameObject* child : children)
    {
        RecursiveDelete(child);
    }

    // Eliminar este GameObject
    delete go;
}

void ModuleScene::ClearScene()
{
    if (selectedGameObject)
    {
        selectedGameObject = nullptr;
    }

    // Eliminar todo excepto el root
    if (root)
    {
        std::vector<GameObject*> children = root->GetChildren();
        for (GameObject* child : children)
        {
            RecursiveDelete(child);
        }

        // Limpiar el root
        delete root;
        root = nullptr;
    }

    allGameObjects.clear();

    std::cout << "[ModuleScene] Scene cleared" << std::endl;
}

void ModuleScene::LoadModel(const char* path)
{
    std::cout << "[ModuleScene] Loading model: " << path << std::endl;

    // NO borrar nada previamente: cada modelo se añade al root
    if (!root)
    {
        root = new GameObject("Scene Root");
        allGameObjects.push_back(root);
    }

    // Crear Assimp importer
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

    // Cada modelo se añade al root, como nuevo GameObject
    LoadFromAssimp(scene, scene->mRootNode, root, basePath);

    std::cout << "[ModuleScene] Model hierarchy loaded" << std::endl;

    // IMPORTANTE: no destruir el importer hasta que termines de usar la escena
    // (o usar Assimp::Importer como variable local, no puntero)
    delete importer;
}


void ModuleScene::LoadFromAssimp(const aiScene* scene, const aiNode* node, GameObject* parent, const std::string& basePath)
{
    // Crear GameObject para este nodo
    GameObject* gameObject = CreateGameObject(node->mName.C_Str(), parent);

    // COMPONENTE TRANSFORM (obligatorio siempre)
    ComponentTransform* transform = (ComponentTransform*)gameObject->CreateComponent(ComponentType::TRANSFORM);

    // Descomponer la transformación del nodo de Assimp
    aiVector3D translation, scaling;
    aiQuaternion rotation;
    node->mTransformation.Decompose(scaling, rotation, translation);

    // Convertir a glm
    glm::vec3 pos(translation.x, translation.y, translation.z);
    glm::vec3 scale(scaling.x, scaling.y, scaling.z);
    glm::quat rot(rotation.w, rotation.x, rotation.y, rotation.z); // glm usa (w,x,y,z)

    transform->SetPosition(pos);
    transform->SetScale(scale);
    transform->SetRotation(rot);

    std::cout << "[ModuleScene] Created GameObject: " << node->mName.C_Str()
        << " with transform" << std::endl;

    // COMPONENTES MESH Y MATERIAL (si el nodo tiene meshes)
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        unsigned int meshIndex = node->mMeshes[i];
        const aiMesh* mesh = scene->mMeshes[meshIndex];

        std::cout << "[ModuleScene] - Processing mesh " << i << ": " << mesh->mName.C_Str() << std::endl;

        // Si hay múltiples meshes en un nodo, crear un hijo por cada uno
        GameObject* meshGameObject = gameObject;
        if (i > 0)
        {
            std::string meshName = std::string(node->mName.C_Str()) + "_mesh_" + std::to_string(i);
            meshGameObject = CreateGameObject(meshName.c_str(), gameObject);

            // Transform identity para los sub-meshes
            ComponentTransform* subTransform = (ComponentTransform*)meshGameObject->CreateComponent(ComponentType::TRANSFORM);
            subTransform->SetPosition(glm::vec3(0.0f));
            subTransform->SetScale(glm::vec3(1.0f));
            subTransform->SetRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        }

        // COMPONENTE MESH
        ComponentMesh* compMesh = (ComponentMesh*)meshGameObject->CreateComponent(ComponentType::MESH);
        compMesh->LoadMesh(mesh);

        std::cout << "[ModuleScene]   - Loaded mesh with "
            << mesh->mNumVertices << " vertices and "
            << mesh->mNumFaces << " faces" << std::endl;

        // COMPONENTE MATERIAL (textura)
        ComponentMaterial* compMaterial = (ComponentMaterial*)meshGameObject->CreateComponent(ComponentType::MATERIAL);

        // Intentar cargar textura desde el material de Assimp
        if (mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < scene->mNumMaterials)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            // Intentar obtener textura difusa
            // Intentar obtener textura difusa
            unsigned int numTextures = material->GetTextureCount(aiTextureType_DIFFUSE);

            if (numTextures > 0)
            {
                aiString texPath;
                if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
                {
                    // Construir path completo
                    std::string fullPath = basePath + "/" + std::string(texPath.C_Str());

                    std::cout << "[ModuleScene]   - Loading texture: " << fullPath << std::endl;
                    compMaterial->LoadTexture(fullPath.c_str());

                    // Path alternativo (solo el nombre del archivo, por si está en otra carpeta)
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

    // RECURSIÓN: Procesar todos los hijos del nodo
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        LoadFromAssimp(scene, node->mChildren[i], gameObject, basePath);
    }
}