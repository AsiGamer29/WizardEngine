#pragma once
#include <glm/glm.hpp>
#include <array>
#include "AABB.h"

class Frustum
{
public:
    enum Plane
    {
        Left = 0,
        Right,
        Bottom,
        Top,
        Near,
        Far,
        Count
    };

    struct FrustumPlane
    {
        glm::vec3 normal;
        float distance;

        FrustumPlane() : normal(0.0f), distance(0.0f) {}

        void Normalize()
        {
            float length = glm::length(normal);
            normal /= length;
            distance /= length;
        }

        float DistanceToPoint(const glm::vec3& point) const
        {
            return glm::dot(normal, point) + distance;
        }
    };

    Frustum() = default;

    void Update(const glm::mat4& viewProjectionMatrix);

    bool IsBoxVisible(const AABB& aabb) const;

    bool IsSphereVisible(const glm::vec3& center, float radius) const;

    bool IsPointVisible(const glm::vec3& point) const;

    const std::array<FrustumPlane, Plane::Count>& GetPlanes() const { return planes; }

private:
    std::array<FrustumPlane, Plane::Count> planes;
};
