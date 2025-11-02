#pragma once
#include <vector>
#include <string>
#include <memory>

class Component;
enum class ComponentType;

class GameObject
{
private:
    std::string name;
    bool active;

    GameObject* parent;
    std::vector<GameObject*> children;
    std::vector<Component*> components;

public:
    GameObject(const char* name);
    ~GameObject();

    void Update();

    // Gestión de componentes
    Component* CreateComponent(ComponentType type);
    Component* GetComponent(ComponentType type);
    void AddComponent(Component* component);
    template<typename T>
    
    T* GetComponent() {
        for (Component* comp : components) {
            T* castedComp = dynamic_cast<T*>(comp);
            if (castedComp) {
                return castedComp;
            }
        }
        return nullptr;
    }
    

    // Gestión de jerarquía
    void AddChild(GameObject* child);
    void RemoveChild(GameObject* child);
    void SetParent(GameObject* newParent);
    GameObject* GetParent() const { return parent; }
    const std::vector<GameObject*>& GetChildren() const { return children; }

    // Getters/Setters
    const char* GetName() const { return name.c_str(); }
    void SetName(const char* newName) { name = newName; }
    bool IsActive() const { return active; }
    void SetActive(bool state) { active = state; }
};