#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : position(position), worldUp(up), yaw(yaw), pitch(pitch),
    movementSpeed(5.0f), baseMovementSpeed(5.0f), mouseSensitivity(0.1f),
    zoom(45.0f), fov(45.0f), aspectRatio(16.0f / 9.0f),
    nearPlane(0.1f), farPlane(1000.0f)
{
    front = glm::vec3(0.0f, 0.0f, -1.0f);
    updateCameraVectors();
}

void Camera::update(Input* input, float deltaTime)
{
    if (!input) return;

    // CRITICAL: Solo procesar input de camara si el viewport esta hovered
    bool viewportHovered = input->IsViewportHovered();

    processKeyboard(input, deltaTime);

    bool rightMousePressed = (input->GetMouseButton(3) == KEY_DOWN || input->GetMouseButton(3) == KEY_REPEAT);
    bool leftMousePressed = (input->GetMouseButton(1) == KEY_DOWN || input->GetMouseButton(1) == KEY_REPEAT);

    SDL_Point motion = input->GetMouseMotion();
    float xoffset = static_cast<float>(motion.x);
    float yoffset = static_cast<float>(motion.y);

    if (rightMousePressed && viewportHovered)
    {
        if (!wasRightMousePressed)
        {
            wasRightMousePressed = true;
        }
        processMouseMovement(xoffset, yoffset);
    }
    else
    {
        wasRightMousePressed = false;
    }

    if (leftMousePressed && viewportHovered)
    {
        if (!wasLeftMousePressed)
        {
            wasLeftMousePressed = true;
            orbitMode = true;
            orbitTarget = position + front * orbitDistance;
        }
        processOrbitMovement(xoffset, yoffset);
    }
    else
    {
        wasLeftMousePressed = false;
        orbitMode = false;
    }

    // CRITICAL: Solo procesar scroll wheel si el viewport esta hovered
    if (viewportHovered)
    {
        int wheel = input->GetMouseWheel();
        if (wheel != 0)
        {
            processMouseScroll(static_cast<float>(wheel));
        }
    }
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix() const
{
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

void Camera::setProjection(float fov, float aspect, float nearP, float farP)
{
    this->fov = fov;
    this->aspectRatio = aspect;
    this->nearPlane = nearP;
    this->farPlane = farP;
}

Ray Camera::ScreenPointToRay(float mouseX, float mouseY, int screenWidth, int screenHeight)
{
    float ndcX = (2.0f * mouseX) / screenWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY) / screenHeight;

    glm::vec4 clipCoords(ndcX, ndcY, -1.0f, 1.0f);

    glm::mat4 invProj = glm::inverse(getProjectionMatrix());
    glm::vec4 eyeCoords = invProj * clipCoords;
    eyeCoords = glm::vec4(eyeCoords.x, eyeCoords.y, -1.0f, 0.0f);

    glm::mat4 invView = glm::inverse(getViewMatrix());
    glm::vec4 worldCoords = invView * eyeCoords;
    glm::vec3 rayDirection = glm::normalize(glm::vec3(worldCoords));

    return Ray(position, rayDirection);
}

void Camera::updateCameraVectors()
{
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::processKeyboard(Input* input, float deltaTime)
{
    bool isSprinting = (input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_DOWN ||
        input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_REPEAT);

    float velocity = (isSprinting ? baseMovementSpeed * sprintMultiplier : baseMovementSpeed) * deltaTime;

    if (input->GetKey(SDL_SCANCODE_W) == KEY_DOWN || input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT)
        position += front * velocity;
    if (input->GetKey(SDL_SCANCODE_S) == KEY_DOWN || input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT)
        position -= front * velocity;
    if (input->GetKey(SDL_SCANCODE_A) == KEY_DOWN || input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)
        position -= right * velocity;
    if (input->GetKey(SDL_SCANCODE_D) == KEY_DOWN || input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT)
        position += right * velocity;
    if (input->GetKey(SDL_SCANCODE_Q) == KEY_DOWN || input->GetKey(SDL_SCANCODE_Q) == KEY_REPEAT)
        position -= up * velocity;
    if (input->GetKey(SDL_SCANCODE_E) == KEY_DOWN || input->GetKey(SDL_SCANCODE_E) == KEY_REPEAT)
        position += up * velocity;
}

void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch -= yoffset;

    if (constrainPitch)
    {
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset)
{
    float zoomSpeed = 2.0f;
    position += front * yoffset * zoomSpeed;
}

void Camera::processOrbitMovement(float xoffset, float yoffset)
{
    if (!orbitMode) return;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch -= yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    updateCameraVectors();

    position = orbitTarget - front * orbitDistance;
}