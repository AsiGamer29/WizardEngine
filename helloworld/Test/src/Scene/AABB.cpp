#include "AABB.h"
#include "Ray.h"
#include <algorithm>
#include <limits>

AABB::AABB()
    : min(std::numeric_limits<float>::max())
    , max(std::numeric_limits<float>::lowest())
{
}

AABB::AABB(const glm::vec3& min, const glm::vec3& max)
    : min(min)
    , max(max)
{
}

bool AABB::IsValid() const
{
    return min.x <= max.x && min.y <= max.y && min.z <= max.z;
}

void AABB::Reset()
{
    min = glm::vec3(std::numeric_limits<float>::max());
    max = glm::vec3(std::numeric_limits<float>::lowest());
}

void AABB::ExpandToInclude(const glm::vec3& point)
{
    min = glm::min(min, point);
    max = glm::max(max, point);
}

// Alias para compatibilidad si tu código usa Encapsulate
void AABB::Encapsulate(const glm::vec3& point)
{
    ExpandToInclude(point);
}

void AABB::ExpandToInclude(const AABB& other)
{
    if (!other.IsValid())
        return;

    min = glm::min(min, other.min);
    max = glm::max(max, other.max);
}

glm::vec3 AABB::GetCenter() const
{
    return (min + max) * 0.5f;
}

glm::vec3 AABB::GetSize() const
{
    return max - min;
}

glm::vec3 AABB::GetHalfSize() const
{
    return GetSize() * 0.5f;
}

float AABB::GetVolume() const
{
    if (!IsValid())
        return 0.0f;

    glm::vec3 size = GetSize();
    return size.x * size.y * size.z;
}

bool AABB::Contains(const glm::vec3& point) const
{
    return point.x >= min.x && point.x <= max.x &&
        point.y >= min.y && point.y <= max.y &&
        point.z >= min.z && point.z <= max.z;
}

bool AABB::Intersects(const AABB& other) const
{
    if (!IsValid() || !other.IsValid())
        return false;

    return (min.x <= other.max.x && max.x >= other.min.x) &&
        (min.y <= other.max.y && max.y >= other.min.y) &&
        (min.z <= other.max.z && max.z >= other.min.z);
}

bool AABB::IntersectsRay(const Ray& ray, float& tMin, float& tMax) const
{
    if (!IsValid())
        return false;

    tMin = -std::numeric_limits<float>::infinity();
    tMax = std::numeric_limits<float>::infinity();

    // Prueba de intersección para cada eje
    for (int i = 0; i < 3; ++i)
    {
        if (std::abs(ray.direction[i]) < 1e-6f)
        {
            // Rayo paralelo a este plano
            if (ray.origin[i] < min[i] || ray.origin[i] > max[i])
                return false;
        }
        else
        {
            float t1 = (min[i] - ray.origin[i]) / ray.direction[i];
            float t2 = (max[i] - ray.origin[i]) / ray.direction[i];

            tMin = std::max(tMin, std::min(t1, t2));
            tMax = std::min(tMax, std::max(t1, t2));
        }
    }

    return tMax >= tMin && tMax >= 0.0f;
}

void AABB::GetCorners(glm::vec3* corners) const
{
    // Orden de esquinas: 8 vértices de un cubo
    // Bottom face (min.y)
    corners[0] = glm::vec3(min.x, min.y, min.z);
    corners[1] = glm::vec3(max.x, min.y, min.z);
    corners[4] = glm::vec3(min.x, min.y, max.z);
    corners[5] = glm::vec3(max.x, min.y, max.z);

    // Top face (max.y)
    corners[2] = glm::vec3(min.x, max.y, min.z);
    corners[3] = glm::vec3(max.x, max.y, min.z);
    corners[6] = glm::vec3(min.x, max.y, max.z);
    corners[7] = glm::vec3(max.x, max.y, max.z);
}

AABB AABB::Transform(const glm::mat4& transform) const
{
    if (!IsValid())
        return AABB();

    // Obtener las 8 esquinas del AABB
    glm::vec3 corners[8];
    GetCorners(corners);

    // Transformar la primera esquina para inicializar
    glm::vec4 transformedCorner = transform * glm::vec4(corners[0], 1.0f);
    AABB result;
    result.min = glm::vec3(transformedCorner);
    result.max = result.min;

    // Transformar el resto de esquinas y expandir el AABB
    for (int i = 1; i < 8; i++)
    {
        transformedCorner = transform * glm::vec4(corners[i], 1.0f);
        glm::vec3 transformedPoint = glm::vec3(transformedCorner);

        result.ExpandToInclude(transformedPoint);
    }

    return result;
}

AABB AABB::Merge(const AABB& a, const AABB& b)
{
    if (!a.IsValid())
        return b;
    if (!b.IsValid())
        return a;

    AABB result;
    result.min = glm::min(a.min, b.min);
    result.max = glm::max(a.max, b.max);
    return result;
}

float AABB::DistanceToPoint(const glm::vec3& point) const
{
    if (!IsValid())
        return std::numeric_limits<float>::infinity();

    // Encontrar el punto más cercano en el AABB
    glm::vec3 closestPoint = ClosestPoint(point);
    return glm::length(point - closestPoint);
}

glm::vec3 AABB::ClosestPoint(const glm::vec3& point) const
{
    glm::vec3 result;
    result.x = std::max(min.x, std::min(point.x, max.x));
    result.y = std::max(min.y, std::min(point.y, max.y));
    result.z = std::max(min.z, std::min(point.z, max.z));
    return result;
}