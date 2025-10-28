#pragma once
#include "Input.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    Camera(glm::vec3 position, glm::vec3 up, float yaw = -90.0f, float pitch = 0.0f);

    void update(Input* input, float deltaTime);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    glm::vec3 getPosition() const { return position; }

    void setProjection(float fov, float aspect, float nearP, float farP);

private:
    void updateCameraVectors();
    void processKeyboard(Input* input, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void processMouseScroll(float yoffset);

private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;

    float movementSpeed;
    float baseMovementSpeed;
    float sprintMultiplier = 2.0f;
    float mouseSensitivity;

    float zoom;
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;

    bool wasRightMousePressed = false;
    bool wasLeftMousePressed = false;

    bool orbitMode = false;
    glm::vec3 orbitTarget;
    float orbitDistance = 5.0f;
};
