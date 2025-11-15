#include "Camera.h"
#include "Input.h"
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3/SDL.h>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : position(position), worldUp(up), yaw(yaw), pitch(pitch),
    front(glm::vec3(0, 0, -1)), baseMovementSpeed(2.5f), movementSpeed(2.5f),
    sprintMultiplier(2.0f), mouseSensitivity(0.1f),
    zoom(45.0f), fov(45.0f), aspectRatio(16.0f / 9.0f),
    nearPlane(0.1f), farPlane(1000.0f),
    orbitDistance(5.0f), orbitTarget(glm::vec3(0.0f))
{
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix() const
{
    return glm::perspective(glm::radians(zoom), aspectRatio, nearPlane, farPlane);
}

void Camera::setProjection(float fov, float aspect, float nearP, float farP)
{
    this->fov = fov;
    this->aspectRatio = aspect;
    this->nearPlane = nearP;
    this->farPlane = farP;
    zoom = fov;
}

void Camera::update(Input* input, float deltaTime)
{
    if (!input) return;

    bool rightMouse = input->GetMouseButton(3) == KEY_DOWN || input->GetMouseButton(3) == KEY_REPEAT; // botón derecho
    bool leftMouse = input->GetMouseButton(1) == KEY_DOWN || input->GetMouseButton(1) == KEY_REPEAT; // botón izquierdo
    bool altPressed = input->GetKey(SDL_SCANCODE_LALT) == KEY_DOWN || input->GetKey(SDL_SCANCODE_LALT) == KEY_REPEAT ||
        input->GetKey(SDL_SCANCODE_RALT) == KEY_DOWN || input->GetKey(SDL_SCANCODE_RALT) == KEY_REPEAT;

    float currentSpeed = (input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_REPEAT || input->GetKey(SDL_SCANCODE_RSHIFT) == KEY_REPEAT)
        ? baseMovementSpeed * sprintMultiplier
        : baseMovementSpeed;

    float velocity = currentSpeed * deltaTime;
    SDL_Point motion = input->GetMouseMotion();
    int wheel = input->GetMouseWheel();

    // --- ORBIT ---
    if (altPressed && leftMouse)
    {
        if (!wasLeftMousePressed) {
            orbitTarget = position + front * orbitDistance;
        }
        processOrbitMovement(static_cast<float>(motion.x), static_cast<float>(motion.y));
    }

    // --- FREE LOOK ---
    else if (rightMouse)
    {
        processMouseMovement(static_cast<float>(motion.x), -static_cast<float>(motion.y));
        processKeyboard(input, velocity);
    }

    // --- ZOOM ---
    if (wheel != 0)
        processMouseScroll(static_cast<float>(wheel));

    wasRightMousePressed = rightMouse;
    wasLeftMousePressed = leftMouse;
}


void Camera::processKeyboard(Input* input, float velocity)
{
    if (input->GetKey(SDL_SCANCODE_W) == KEY_DOWN || input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT)
        position += front * velocity;
    if (input->GetKey(SDL_SCANCODE_S) == KEY_DOWN || input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT)
        position -= front * velocity;
    if (input->GetKey(SDL_SCANCODE_A) == KEY_DOWN || input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)
        position -= right * velocity;
    if (input->GetKey(SDL_SCANCODE_D) == KEY_DOWN || input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT)
        position += right * velocity;
}

void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (constrainPitch)
    {
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::processOrbitMovement(float xoffset, float yoffset)
{
    xoffset *= -mouseSensitivity;
    yoffset *= -mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    direction.y = sin(glm::radians(pitch));
    direction.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));

    position = orbitTarget - glm::normalize(direction) * orbitDistance;

    front = glm::normalize(orbitTarget - position);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}



void Camera::processMouseScroll(float yoffset)
{
    zoom -= yoffset * 2.0f;
    if (zoom < 1.0f) zoom = 1.0f;
    if (zoom > 90.0f) zoom = 90.0f;
}

void Camera::updateCameraVectors()
{
    glm::vec3 f;
    f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    f.y = sin(glm::radians(pitch));
    f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(f);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

Ray Camera::ScreenPointToRay(float mouseX, float mouseY, int screenWidth, int screenHeight)
{
    // Normalizar coordenadas del mouse a [-1, 1]
    float x = (2.0f * mouseX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenHeight;  // Invertir Y

    // Crear punto en Near Plane (NDC)
    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);

    // Transformar a Eye Space
    glm::mat4 projInverse = glm::inverse(getProjectionMatrix());
    glm::vec4 rayEye = projInverse * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    // Transformar a World Space
    glm::mat4 viewInverse = glm::inverse(getViewMatrix());
    glm::vec4 rayWorld = viewInverse * rayEye;
    glm::vec3 rayDirection = glm::normalize(glm::vec3(rayWorld));

    return Ray(position, rayDirection);
}