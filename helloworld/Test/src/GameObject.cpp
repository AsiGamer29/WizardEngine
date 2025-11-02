#include "GameObject.h"
#include "BaseComponent.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include <iostream>

GameObject::GameObject(const char* name)
    : name(name), active(true), parent(nullptr)
{
}

GameObject::~GameObject()
{
    // Limpiar componentes
    for (Component* comp : components)
    {
        delete comp;
    }
    components.clear();

    // No eliminamos hijos aquí, lo hace ModuleScene::RecursiveDelete
}

void GameObject::Update()
{
    if (!active)
        return;

    // Actualizar componentes
    for (Component* comp : components)
    {
        if (comp->IsActive())
        {
            comp->Update();
        }
    }

    // Actualizar hijos
    for (GameObject* child : children)
    {
        child->Update();
    }
}

Component* GameObject::CreateComponent(ComponentType type)
{
    Component* newComponent = nullptr;

    switch (type)
    {
    case ComponentType::TRANSFORM:
        newComponent = new ComponentTransform(this);
        break;

    case ComponentType::MESH:
        newComponent = new ComponentMesh(this);
        break;

    case ComponentType::MATERIAL:
        newComponent = new ComponentMaterial(this);
        break;

    default:
        std::cerr << "[GameObject] Unknown component type" << std::endl;
        return nullptr;
    }

    components.push_back(newComponent);
    return newComponent;
}

Component* GameObject::GetComponent(ComponentType type)
{
    for (Component* comp : components)
    {
        if (comp->GetType() == type)
            return comp;
    }
    return nullptr;
}

void GameObject::AddChild(GameObject* child)
{
    if (!child)
        return;

    // Si ya tenía padre, quitarlo de su lista de hijos
    if (child->parent)
    {
        child->parent->RemoveChild(child);
    }

    child->parent = this;
    children.push_back(child);
}

void GameObject::RemoveChild(GameObject* child)
{
    if (!child)
        return;

    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end())
    {
        (*it)->parent = nullptr;
        children.erase(it);
    }
}

void GameObject::SetParent(GameObject* newParent)
{
    if (parent)
    {
        parent->RemoveChild(this);
    }

    if (newParent)
    {
        newParent->AddChild(this);
    }
}
void GameObject::AddComponent(Component* component)
{
    if (component == nullptr)
    {
        std::cerr << "[GameObject] Cannot add null component" << std::endl;
        return;
    }

    // Verificar que no exista ya
    auto it = std::find(components.begin(), components.end(), component);
    if (it != components.end())
    {
        std::cerr << "[GameObject] Component already exists in GameObject" << std::endl;
        return;
    }

    components.push_back(component);
}