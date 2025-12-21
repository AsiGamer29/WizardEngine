#pragma once
#include "BaseComponent.h"
#include "ComponentSerializer.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class ComponentCamera : public Component, public ComponentSerializer
{
public:
    ComponentCamera(GameObject* owner);
    ~ComponentCamera();

    void Enable() override;
    void Update() override;
    void Disable() override;
    void OnEditor() override;

    // Camera properties
    float GetFOV() const { return fov; }
    void SetFOV(float newFov) { fov = newFov; }

    float GetNearPlane() const { return nearPlane; }
    void SetNearPlane(float nearP) { nearPlane = nearP; }

    float GetFarPlane() const { return farPlane; }
    void SetFarPlane(float farP) { farPlane = farP; }

    float GetAspectRatio() const { return aspectRatio; }
    void SetAspectRatio(float aspect) { aspectRatio = aspect; }

    // Matrix calculations
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;

    // Camera mode
    enum class ProjectionMode
    {
        PERSPECTIVE,
        ORTHOGRAPHIC
    };

    ProjectionMode GetProjectionMode() const { return projectionMode; }
    void SetProjectionMode(ProjectionMode mode) { projectionMode = mode; }

    // Orthographic settings
    float GetOrthoSize() const { return orthoSize; }
    void SetOrthoSize(float size) { orthoSize = size; }

    // Check if this camera is the main/active one
    bool IsMainCamera() const { return isMainCamera; }
    void SetMainCamera(bool main) { isMainCamera = main; }

    // Background color
    glm::vec3 GetBackgroundColor() const { return backgroundColor; }
    void SetBackgroundColor(const glm::vec3& color) { backgroundColor = color; }
    nlohmann::json GetProperties() const override;
    void SetProperties(const nlohmann::json& props) override;
private:
    // Camera parameters
    float fov;
    float nearPlane;
    float farPlane;
    float aspectRatio;

    // Projection mode
    ProjectionMode projectionMode;

    // Orthographic settings
    float orthoSize;

    // Main camera flag
    bool isMainCamera;

    // Background color
    glm::vec3 backgroundColor;
};