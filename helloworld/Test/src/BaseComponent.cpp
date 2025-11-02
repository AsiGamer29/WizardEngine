#include "BaseComponent.h"
#include "GameObject.h"

Component::Component(GameObject* owner, ComponentType type)
    : owner(owner), type(type), active(true)
{
}

Component::~Component()
{
}