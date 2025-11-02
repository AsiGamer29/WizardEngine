#pragma once
#include "Module.h"
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
    bool CleanUp() override;

    // Gestión de GameObjects
    GameObject* CreateGameObject(const char* name, GameObject* parent = nullptr);
    void DestroyGameObject(GameObject* gameObject);

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

private:
    void LoadFromAssimp(const aiScene* scene, const aiNode* node, GameObject* parent, const std::string& basePath);
    void RecursiveDelete(GameObject* go);
};