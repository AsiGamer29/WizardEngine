#pragma once
#include <string>

class GameObject;

enum class ComponentType
{
    TRANSFORM,
    MESH,
    MATERIAL,
    CAMERA,
    LIGHT
};

class Component
{
protected:
    ComponentType type;
    bool active;
    GameObject* owner;

public:
    Component(GameObject* owner, ComponentType type);
    virtual ~Component();

    virtual void Enable() {}
    virtual void Update() {}
    virtual void Disable() {}
    virtual void OnEditor() {} // Para ImGui inspector

    // Getters
    ComponentType GetType() const { return type; }
    bool IsActive() const { return active; }
    GameObject* GetOwner() const { return owner; }

    // Setters
    void SetActive(bool state) { active = state; }
};