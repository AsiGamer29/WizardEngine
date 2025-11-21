#include "ComponentTransform.h"
#include "GameObject.h"
#include "imgui.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

ComponentTransform::ComponentTransform(GameObject* owner)
    : Component(owner, ComponentType::TRANSFORM),
    position(0.0f),
    rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
    scale(1.0f),
    matrixNeedsUpdate(true)
{
}

ComponentTransform::~ComponentTransform()
{
}

void ComponentTransform::SetPosition(const glm::vec3& pos)
{
    position = pos;
    matrixNeedsUpdate = true;
}

void ComponentTransform::SetRotation(const glm::quat& rot)
{
    rotation = rot;
    matrixNeedsUpdate = true;
}

void ComponentTransform::SetScale(const glm::vec3& scl)
{
    scale = scl;
    matrixNeedsUpdate = true;
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

glm::mat4 ComponentTransform::GetLocalMatrix() const
{
    // Construir matriz local: Translate * Rotate * Scale
    glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 R = glm::mat4_cast(rotation);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);

    return T * R * S;
}

glm::mat4 ComponentTransform::GetGlobalMatrix() const
{
    glm::mat4 localMat = GetLocalMatrix();

    // CRÍTICO: Si tiene padre, multiplicar por la matriz global del padre
    GameObject* parent = owner->GetParent();
    if (parent)
    {
        ComponentTransform* parentTransform = parent->GetComponent<ComponentTransform>();
        if (parentTransform)
        {
            // Matriz global = Matriz_del_Padre * Matriz_Local
            return parentTransform->GetGlobalMatrix() * localMat;
        }
    }

    // Si no hay padre, la matriz global = matriz local
    return localMat;
}

glm::vec3 ComponentTransform::GetGlobalPosition() const
{
    glm::mat4 globalMat = GetGlobalMatrix();
    return glm::vec3(globalMat[3]);
}

glm::quat ComponentTransform::GetGlobalRotation() const
{
    glm::mat4 globalMat = GetGlobalMatrix();

    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;

    glm::decompose(globalMat, scale, rotation, translation, skew, perspective);

    return rotation;
}

glm::vec3 ComponentTransform::GetGlobalScale() const
{
    glm::mat4 globalMat = GetGlobalMatrix();

    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;

    glm::decompose(globalMat, scale, rotation, translation, skew, perspective);

    return scale;
}

void ComponentTransform::OnEditor()
{
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        float pos[3] = { position.x, position.y, position.z };
        if (ImGui::DragFloat3("Position", pos, 0.1f))
        {
            SetPosition(glm::vec3(pos[0], pos[1], pos[2]));
        }

        glm::vec3 euler = glm::degrees(glm::eulerAngles(rotation));
        float rot[3] = { euler.x, euler.y, euler.z };
        if (ImGui::DragFloat3("Rotation", rot, 1.0f))
        {
            glm::vec3 radians = glm::radians(glm::vec3(rot[0], rot[1], rot[2]));
            SetRotation(glm::quat(radians));
        }

        float scl[3] = { scale.x, scale.y, scale.z };
        if (ImGui::DragFloat3("Scale", scl, 0.01f))
        {
            SetScale(glm::vec3(scl[0], scl[1], scl[2]));
        }

        // DEBUG: Mostrar posición global
        ImGui::Separator();
        glm::vec3 globalPos = GetGlobalPosition();
        ImGui::Text("Global Position: (%.2f, %.2f, %.2f)",
            globalPos.x, globalPos.y, globalPos.z);
    }
}