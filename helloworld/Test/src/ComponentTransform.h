#pragma once
#include "BaseComponent.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class ComponentTransform : public Component
{
private:
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    mutable glm::mat4 localMatrix;
    mutable glm::mat4 globalMatrix;
    mutable bool localMatrixDirty;
    mutable bool globalMatrixDirty;

public:
    ComponentTransform(GameObject* owner);
    ~ComponentTransform();

    void SetPosition(const glm::vec3& pos);
    void SetRotation(const glm::quat& rot);
    void SetScale(const glm::vec3& scl);

    glm::vec3 GetPosition() const;
    glm::quat GetRotation() const;
    glm::vec3 GetScale() const;

    glm::mat4 GetLocalMatrix() const;
    glm::mat4 GetGlobalMatrix() const;

    glm::vec3 GetGlobalPosition() const;
    glm::quat GetGlobalRotation() const;
    glm::vec3 GetGlobalScale() const;

    void MarkDirty();
    void MarkGlobalDirty();

    void OnEditor() override;

private:
    void UpdateLocalMatrix() const;
    void UpdateGlobalMatrix() const;
    void PropagateGlobalDirtyToChildren();
};