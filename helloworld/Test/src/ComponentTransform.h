#pragma once
#include "BaseComponent.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

class ComponentTransform : public Component
{
private:
    // Transformación local
    glm::vec3 position;
    glm::vec3 scale;
    glm::quat rotation;

    // Matrices
    glm::mat4 localMatrix;
    glm::mat4 globalMatrix;

    bool isDirty; // Flag para lazy evaluation

public:
    ComponentTransform(GameObject* owner);
    ~ComponentTransform();

    void Update() override;
    void OnEditor() override;

    // Setters (marcan isDirty = true)
    void SetPosition(const glm::vec3& pos);
    void SetRotation(const glm::quat& rot);
    void SetScale(const glm::vec3& scl);

    // Getters
    glm::vec3 GetPosition() const { return position; }
    glm::quat GetRotation() const { return rotation; }
    glm::vec3 GetScale() const { return scale; }

    // Matrices
    glm::mat4 GetLocalMatrix();
    glm::mat4 GetGlobalMatrix();

private:
    void UpdateLocalMatrix();
    void UpdateGlobalMatrix();
};