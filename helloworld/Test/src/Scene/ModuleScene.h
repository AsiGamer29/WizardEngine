#pragma once
#include "Module.h"
#include "AssetManager.h"
#include "Ray.h"
#include <vector>
#include <string>

class GameObject;
struct aiScene;
struct aiNode;

class ModuleScene : public Module
{
private:
    GameObject* root;
    GameObject* selectedGameObject; // Para el inspector
    std::vector<GameObject*> allGameObjects; // Todos los GOs para facilitar búsqueda

    // Debug visualization flags
    bool debugShowNormals = false;

public:
    ModuleScene();
    ~ModuleScene();

    bool Start() override;
    bool PreUpdate() override;
    bool Update() override;
    bool PostUpdate() override;
    void RenderScene();
    bool CleanUp() override;

    // Gestión de GameObjects
    GameObject* CreateGameObject(const char* name, GameObject* parent = nullptr);
    void DestroyGameObject(GameObject* gameObject);
    GameObject* DuplicateGameObject(GameObject* original);

    // Carga desde Assimp (modelo 3D)
    void LoadModel(const char* path);

    // Limpia toda la escena
    void ClearScene();

    // Getters
    GameObject* GetRoot() const { return root; }
    GameObject* GetSelectedGameObject() const { return selectedGameObject; }
    void SetSelectedGameObject(GameObject* go) { selectedGameObject = go; }
    const std::vector<GameObject*>& GetAllGameObjects() const { return allGameObjects; }

    // Debug flags for editor
    void SetDebugShowNormals(bool v) { debugShowNormals = v; }
    bool GetDebugShowNormals() const { return debugShowNormals; }

    GameObject* PerformRaycast(const Ray& ray);
    void UpdateAllAABBs();

    bool SaveScene(const std::string& filepath);
    bool LoadScene(const std::string& filepath);
    void SetRoot(GameObject* newRoot) { root = newRoot; }

    GameObject* ImportModelFromAssets(const std::string& assetPath);

    GameObject* LoadModelFromLibrary(const std::string& libraryPath);

    WizardEngine::AssetManager* GetAssetManager() { return assetManager; }

    GameObject* LoadModelFromAssetPath(const std::string& assetPath);

private:
    void LoadFromAssimp(const aiScene* scene, const aiNode* node, GameObject* parent, const std::string& basePath);
    void CollectRaycastCandidates(GameObject* go, const Ray& ray, std::vector<RayHit>& candidates);
    std::string currentScenePath;
    WizardEngine::AssetManager* assetManager;
};