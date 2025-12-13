#pragma once
#include <string>
#include <vector>
#include "BaseComponent.h"
#include "AABB.h"
#include "UUID.h"
#include <nlohmann/json.hpp>
#include <glm/glm.hpp>

// Forward declarations
class Ray;
struct RayHit;
struct AABB;

class GameObject
{
public:
    GameObject(const char* name, GameObject* parent = nullptr);
    ~GameObject();

    // Component management
    Component* CreateComponent(ComponentType type);

    template<typename T>
    T* GetComponent() const
    {
        for (Component* comp : components)
        {
            if (T* result = dynamic_cast<T*>(comp))
                return result;
        }
        return nullptr;
    }

    // Hierarchy
    void SetParent(GameObject* newParent);
    void AddChild(GameObject* child);
    void RemoveChild(GameObject* child);

    // Getters/Setters
    const char* GetName() const { return name.c_str(); }
    void SetName(const char* newName) { name = newName; }

    bool IsActive() const { return active; }
    void SetActive(bool state) { active = state; }

    GameObject* GetParent() const { return parent; }
    const std::vector<GameObject*>& GetChildren() const { return children; }
    const std::vector<Component*>& GetComponents() const { return components; }

    // AABB
    void UpdateAABB();
    void SetAABB(const AABB& aabb);
    AABB GetAABB() const { return aabb; }
    bool HasAABB() const { return hasAABB; }

    // Ray intersection (mouse picking)
    bool IntersectRay(const Ray& ray, RayHit& hit);

    // UUID system
    UUID GetUUID() const { return uuid; }
    void SetUUID(UUID newUuid) { uuid = newUuid; }

    // Serialization
    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& json);
    void ClearHierarchyReferences();

    void Update();

private:
    std::string name;
    bool active;
    GameObject* parent;
    std::vector<GameObject*> children;
    std::vector<Component*> components;

    // AABB for mouse picking
    AABB aabb;
    bool hasAABB;

    // UUID for serialization
    UUID uuid;
};