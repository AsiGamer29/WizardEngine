#pragma once
#include <vector>
#include <string>
#include <memory>
#include "BaseComponent.h"
#include "ComponentMesh.h"
#include "Ray.h"
#include "AABB.h"

class Component;
struct Ray;
struct RayHit;
class AABB;



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

    AABB globalAABB;  // AABB en espacio global (world space)

    // Helper para intersección con triángulos
    bool IntersectRayTriangles(const Ray& rayLocal, ComponentMesh* mesh, float& closestDist, glm::vec3& hitPoint);
    

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
    
    // Sistema de AABB
    void UpdateAABB();
    const AABB& GetAABB() const { return globalAABB; }

    // Gestión de jerarquía
    void AddChild(GameObject* child);
    void RemoveChild(GameObject* child);
    void SetParent(GameObject* newParent);
    GameObject* GetParent() const { return parent; }
    const std::vector<GameObject*>& GetChildren() const { return children; }

    bool IntersectRay(const Ray& ray, RayHit& outHit);

    // Getters/Setters
    const char* GetName() const { return name.c_str(); }
    void SetName(const char* newName) { name = newName; }
    bool IsActive() const { return active; }
    void SetActive(bool state) { active = state; }
};