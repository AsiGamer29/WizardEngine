#pragma once
#include"GameObject.h"

// Forward declaration
class GameObject;

// Clase base abstracta para todos los componentes
class Component {
protected:
    GameObject* owner;
    bool active;

public:
    Component(GameObject* owner) : owner(owner), active(true) {}
    virtual ~Component() = default;

    // Métodos virtuales puros que deben implementar los componentes derivados
    virtual void update(float deltaTime) = 0;
    virtual void init() = 0;

    // Getters y setters
    GameObject* getOwner() const { return owner; }
    bool isActive() const { return active; }
    void setActive(bool value) { active = value; }
};