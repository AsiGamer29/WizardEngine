#pragma once
#include <glm/glm.hpp>
#include "GameObject.h"

class GameObject;


struct Ray
{
    glm::vec3 origin;
    glm::vec3 direction;

    Ray() : origin(0.0f), direction(0.0f, 0.0f, -1.0f) {}
    Ray(const glm::vec3& origin, const glm::vec3& direction)
        : origin(origin), direction(glm::normalize(direction)) {
    }

    // Obtener un punto a lo largo del rayo
    glm::vec3 GetPoint(float distance) const
    {
        return origin + direction * distance;
    }
};

struct RayHit
{
    bool hit;
    float distance;
    glm::vec3 point;
    GameObject* gameObject;

    RayHit() : hit(false), distance(FLT_MAX), point(0.0f), gameObject(nullptr) {}
};