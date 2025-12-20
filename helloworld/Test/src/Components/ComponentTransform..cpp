#include "ComponentTransform.h"
#include "GameObject.h"
#include "imgui.h"

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

ComponentTransform::ComponentTransform(GameObject* owner)
    : Component(owner, ComponentType::TRANSFORM),
    position(0.0f, 0.0f, 0.0f),
    rotation(1.0f, 0.0f, 0.0f, 0.0f),
    scale(1.0f, 1.0f, 1.0f),
    localMatrix(1.0f),
    globalMatrix(1.0f),
    localMatrixDirty(true),
    globalMatrixDirty(true)
{
}

ComponentTransform::~ComponentTransform()
{
}

void ComponentTransform::SetPosition(const glm::vec3& pos)
{
    if (position != pos)
    {
        position = pos;
        MarkDirty();
    }
}

void ComponentTransform::SetRotation(const glm::quat& rot)
{
    if (rotation != rot)
    {
        rotation = rot;
        MarkDirty();
    }
}

void ComponentTransform::SetScale(const glm::vec3& scl)
{
    if (scale != scl)
    {
        scale = scl;
        MarkDirty();
    }
}

glm::vec3 ComponentTransform::GetPosition() const
{
    return position;
}

glm::quat ComponentTransform::GetRotation() const
{
    return rotation;
}

glm::vec3 ComponentTransform::GetScale() const
{
    return scale;
}

void ComponentTransform::MarkDirty()
{
    localMatrixDirty = true;
    MarkGlobalDirty();
}

void ComponentTransform::MarkGlobalDirty()
{
    globalMatrixDirty = true;
    PropagateGlobalDirtyToChildren();

    // IMPORTANTE: NO actualizar AABB aquí, se hace después de que las matrices estén actualizadas
}

void ComponentTransform::PropagateGlobalDirtyToChildren()
{
    if (!owner) return;

    const std::vector<GameObject*>& children = owner->GetChildren();
    for (GameObject* child : children)
    {
        if (child)
        {
            ComponentTransform* childTransform = child->GetComponent<ComponentTransform>();
            if (childTransform)
            {
                childTransform->MarkGlobalDirty();
            }
        }
    }
}

void ComponentTransform::UpdateLocalMatrix() const
{
    if (!localMatrixDirty) return;

    glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 R = glm::toMat4(rotation);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);

    localMatrix = T * R * S;
    localMatrixDirty = false;
}

void ComponentTransform::UpdateGlobalMatrix() const
{
    if (!globalMatrixDirty) return;

    UpdateLocalMatrix();

    if (owner && owner->GetParent())
    {
        GameObject* parent = owner->GetParent();
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
        globalMatrix = localMatrix;
    }

    globalMatrixDirty = false;
}

glm::mat4 ComponentTransform::GetLocalMatrix() const
{
    UpdateLocalMatrix();
    return localMatrix;
}

glm::mat4 ComponentTransform::GetGlobalMatrix() const
{
    UpdateGlobalMatrix();
    return globalMatrix;
}

glm::vec3 ComponentTransform::GetGlobalPosition() const
{
    glm::mat4 global = GetGlobalMatrix();
    return glm::vec3(global[3]);
}

glm::quat ComponentTransform::GetGlobalRotation() const
{
    glm::mat4 global = GetGlobalMatrix();

    glm::vec3 scale_unused;
    glm::quat rotation_out;
    glm::vec3 translation_unused;
    glm::vec3 skew_unused;
    glm::vec4 perspective_unused;

    glm::decompose(global, scale_unused, rotation_out, translation_unused, skew_unused, perspective_unused);

    return rotation_out;
}

glm::vec3 ComponentTransform::GetGlobalScale() const
{
    glm::mat4 global = GetGlobalMatrix();

    glm::vec3 scale_out;
    glm::quat rotation_unused;
    glm::vec3 translation_unused;
    glm::vec3 skew_unused;
    glm::vec4 perspective_unused;

    glm::decompose(global, scale_out, rotation_unused, translation_unused, skew_unused, perspective_unused);

    return scale_out;
}

void ComponentTransform::OnEditor()
{
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        glm::vec3 pos = GetPosition();
        glm::vec3 scl = GetScale();
        glm::quat rotQ = GetRotation();
        glm::vec3 euler = glm::degrees(glm::eulerAngles(rotQ));

        float posArr[3] = { pos.x, pos.y, pos.z };
        if (ImGui::DragFloat3("Position", posArr, 0.1f))
        {
            SetPosition(glm::vec3(posArr[0], posArr[1], posArr[2]));
            // Actualizar AABB después de cambiar la posición
            if (owner)
            {
                owner->UpdateAABB();
            }
        }

        float rotArr[3] = { euler.x, euler.y, euler.z };
        if (ImGui::DragFloat3("Rotation", rotArr, 1.0f))
        {
            glm::vec3 rads = glm::radians(glm::vec3(rotArr[0], rotArr[1], rotArr[2]));
            glm::quat newQ = glm::quat(rads);
            SetRotation(newQ);
            // Actualizar AABB después de cambiar la rotación
            if (owner)
            {
                owner->UpdateAABB();
            }
        }

        float sclArr[3] = { scl.x, scl.y, scl.z };
        if (ImGui::DragFloat3("Scale", sclArr, 0.01f))
        {
            SetScale(glm::vec3(sclArr[0], sclArr[1], sclArr[2]));
            // Actualizar AABB después de cambiar la escala
            if (owner)
            {
                owner->UpdateAABB();
            }
        }

        ImGui::Separator();
        ImGui::Text("Global Position: (%.2f, %.2f, %.2f)",
            GetGlobalPosition().x, GetGlobalPosition().y, GetGlobalPosition().z);
    }
}