#pragma once
#include <glm/glm.hpp>
#include <algorithm>
#include <cfloat>

// Forward declaration
struct Ray;

class AABB
{
public:
    glm::vec3 min;
    glm::vec3 max;

    // Constructores
    AABB() : min(glm::vec3(FLT_MAX)), max(glm::vec3(-FLT_MAX)) {}

    AABB(const glm::vec3& min, const glm::vec3& max)
        : min(min), max(max) {
    }

    // Resetear el AABB a estado inválido
    void Reset()
    {
        min = glm::vec3(FLT_MAX);
        max = glm::vec3(-FLT_MAX);
    }

    // Expandir AABB para incluir un punto
    void Encapsulate(const glm::vec3& point)
    {
        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        min.z = std::min(min.z, point.z);

        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        max.z = std::max(max.z, point.z);
    }

    // Expandir AABB para incluir otro AABB
    void Encapsulate(const AABB& other)
    {
        Encapsulate(other.min);
        Encapsulate(other.max);
    }

    // Verificar si el AABB es válido
    bool IsValid() const
    {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    // Obtener el centro del AABB
    glm::vec3 GetCenter() const
    {
        return (min + max) * 0.5f;
    }

    // Obtener el tamaño del AABB
    glm::vec3 GetSize() const
    {
        return max - min;
    }

    // Obtener la extensión (mitad del tamaño)
    glm::vec3 GetExtent() const
    {
        return GetSize() * 0.5f;
    }

    // Verificar si contiene un punto
    bool Contains(const glm::vec3& point) const
    {
        return point.x >= min.x && point.x <= max.x &&
            point.y >= min.y && point.y <= max.y &&
            point.z >= min.z && point.z <= max.z;
    }

    // Intersección con otro AABB
    bool Intersects(const AABB& other) const
    {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
            (min.y <= other.max.y && max.y >= other.min.y) &&
            (min.z <= other.max.z && max.z >= other.min.z);
    }

    // Declaraciones (implementación en .cpp para evitar dependencias)
    bool IntersectRay(const Ray& ray, float& tMin, float& tMax) const;
    bool IntersectRay(const Ray& ray) const;

    // Obtener las 8 esquinas del AABB
    void GetCorners(glm::vec3 corners[8]) const
    {
        corners[0] = glm::vec3(min.x, min.y, min.z);
        corners[1] = glm::vec3(max.x, min.y, min.z);
        corners[2] = glm::vec3(max.x, max.y, min.z);
        corners[3] = glm::vec3(min.x, max.y, min.z);
        corners[4] = glm::vec3(min.x, min.y, max.z);
        corners[5] = glm::vec3(max.x, min.y, max.z);
        corners[6] = glm::vec3(max.x, max.y, max.z);
        corners[7] = glm::vec3(min.x, max.y, max.z);
    }

    // Transformar AABB por una matriz
    AABB Transform(const glm::mat4& matrix) const
    {
        glm::vec3 corners[8];
        GetCorners(corners);

        AABB result;
        for (int i = 0; i < 8; ++i)
        {
            glm::vec4 transformed = matrix * glm::vec4(corners[i], 1.0f);
            result.Encapsulate(glm::vec3(transformed));
        }

        return result;
    }

    // Calcular volumen
    float GetVolume() const
    {
        if (!IsValid()) return 0.0f;
        glm::vec3 size = GetSize();
        return size.x * size.y * size.z;
    }

    // Calcular área de superficie
    float GetSurfaceArea() const
    {
        if (!IsValid()) return 0.0f;
        glm::vec3 size = GetSize();
        return 2.0f * (size.x * size.y + size.y * size.z + size.z * size.x);
    }
};