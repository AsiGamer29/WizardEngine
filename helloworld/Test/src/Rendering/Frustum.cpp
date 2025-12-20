#include "Frustum.h"

void Frustum::Update(const glm::mat4& viewProjectionMatrix)
{
    const glm::mat4& m = viewProjectionMatrix;

    // Left plane
    planes[Left].normal.x = m[0][3] + m[0][0];
    planes[Left].normal.y = m[1][3] + m[1][0];
    planes[Left].normal.z = m[2][3] + m[2][0];
    planes[Left].distance = m[3][3] + m[3][0];
    planes[Left].Normalize();

    // Right plane
    planes[Right].normal.x = m[0][3] - m[0][0];
    planes[Right].normal.y = m[1][3] - m[1][0];
    planes[Right].normal.z = m[2][3] - m[2][0];
    planes[Right].distance = m[3][3] - m[3][0];
    planes[Right].Normalize();

    // Bottom plane
    planes[Bottom].normal.x = m[0][3] + m[0][1];
    planes[Bottom].normal.y = m[1][3] + m[1][1];
    planes[Bottom].normal.z = m[2][3] + m[2][1];
    planes[Bottom].distance = m[3][3] + m[3][1];
    planes[Bottom].Normalize();

    // Top plane
    planes[Top].normal.x = m[0][3] - m[0][1];
    planes[Top].normal.y = m[1][3] - m[1][1];
    planes[Top].normal.z = m[2][3] - m[2][1];
    planes[Top].distance = m[3][3] - m[3][1];
    planes[Top].Normalize();

    // Near plane
    planes[Near].normal.x = m[0][3] + m[0][2];
    planes[Near].normal.y = m[1][3] + m[1][2];
    planes[Near].normal.z = m[2][3] + m[2][2];
    planes[Near].distance = m[3][3] + m[3][2];
    planes[Near].Normalize();

    // Far plane
    planes[Far].normal.x = m[0][3] - m[0][2];
    planes[Far].normal.y = m[1][3] - m[1][2];
    planes[Far].normal.z = m[2][3] - m[2][2];
    planes[Far].distance = m[3][3] - m[3][2];
    planes[Far].Normalize();
}

bool Frustum::IsBoxVisible(const AABB& aabb) const
{
    if (!aabb.IsValid())
        return false;

    
    for (const auto& plane : planes)
    {
        
        glm::vec3 positiveVertex = aabb.min;
        if (plane.normal.x >= 0) positiveVertex.x = aabb.max.x;
        if (plane.normal.y >= 0) positiveVertex.y = aabb.max.y;
        if (plane.normal.z >= 0) positiveVertex.z = aabb.max.z;

        
        if (plane.DistanceToPoint(positiveVertex) < 0)
            return false;
    }

    return true;
}

bool Frustum::IsSphereVisible(const glm::vec3& center, float radius) const
{
    for (const auto& plane : planes)
    {
        if (plane.DistanceToPoint(center) < -radius)
            return false;
    }
    return true;
}

bool Frustum::IsPointVisible(const glm::vec3& point) const
{
    for (const auto& plane : planes)
    {
        if (plane.DistanceToPoint(point) < 0)
            return false;
    }
    return true;
}
