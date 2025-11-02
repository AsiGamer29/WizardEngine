#include "ComponentTransform.h"
#include "GameObject.h"
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

ComponentTransform::ComponentTransform(GameObject* owner)
    : Component(owner, ComponentType::TRANSFORM),
    position(0.0f, 0.0f, 0.0f),
    scale(1.0f, 1.0f, 1.0f),
    rotation(1.0f, 0.0f, 0.0f, 0.0f), // Identity quaternion (w, x, y, z)
    isDirty(true)
{
    localMatrix = glm::mat4(1.0f);
    globalMatrix = glm::mat4(1.0f);
}

ComponentTransform::~ComponentTransform()
{
}

void ComponentTransform::Update()
{
    if (isDirty)
    {
        UpdateLocalMatrix();
        UpdateGlobalMatrix();
        isDirty = false;
    }
}

void ComponentTransform::OnEditor()
{
    // TODO: Implementar con ImGui
    // ImGui::DragFloat3("Position", &position.x, 0.1f);
    // ImGui::DragFloat3("Scale", &scale.x, 0.1f);
    // glm::vec3 eulerAngles = glm::eulerAngles(rotation);
    // if (ImGui::DragFloat3("Rotation", &eulerAngles.x, 0.1f))
    // {
    //     rotation = glm::quat(eulerAngles);
    //     isDirty = true;
    // }
}

void ComponentTransform::SetPosition(const glm::vec3& pos)
{
    position = pos;
    isDirty = true;
}

void ComponentTransform::SetRotation(const glm::quat& rot)
{
    rotation = rot;
    isDirty = true;
}

void ComponentTransform::SetScale(const glm::vec3& scl)
{
    scale = scl;
    isDirty = true;
}

glm::mat4 ComponentTransform::GetLocalMatrix()
{
    if (isDirty)
    {
        UpdateLocalMatrix();
    }
    return localMatrix;
}

glm::mat4 ComponentTransform::GetGlobalMatrix()
{
    if (isDirty)
    {
        UpdateLocalMatrix();
        UpdateGlobalMatrix();
        isDirty = false;
    }
    return globalMatrix;
}

void ComponentTransform::UpdateLocalMatrix()
{
    // Crear matriz de transformación: T * R * S
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rotationMatrix = glm::toMat4(rotation);
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

    localMatrix = translationMatrix * rotationMatrix * scaleMatrix;
}

void ComponentTransform::UpdateGlobalMatrix()
{
    // Tu matriz global = matriz global del padre * tu matriz local
    GameObject* parent = owner->GetParent();

    if (parent != nullptr)
    {
        ComponentTransform* parentTransform = parent->GetComponent<ComponentTransform>();
        if (parentTransform)
        {
            globalMatrix = parentTransform->GetGlobalMatrix() * localMatrix;
        }
        else
        {
            globalMatrix = localMatrix;
        }
    }
    else
    {
        // No hay padre, la matriz global es la local
        globalMatrix = localMatrix;
    }
}