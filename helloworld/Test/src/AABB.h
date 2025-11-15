#pragma once
#include <glm/glm.hpp>
#include <algorithm>
#include <cfloat>

struct AABB
{
    glm::vec3 min;
    glm::vec3 max;

    // Constructor por defecto - AABB inválida
    AABB()
        : min(FLT_MAX, FLT_MAX, FLT_MAX),
        max(-FLT_MAX, -FLT_MAX, -FLT_MAX)
    {
    }

    // Constructor con valores
    AABB(const glm::vec3& min, const glm::vec3& max)
        : min(min), max(max)
    {
    }

    // Resetear a AABB inválida
    void Reset()
    {
        min = glm::vec3(FLT_MAX);
        max = glm::vec3(-FLT_MAX);
    }

    // Expandir AABB para incluir un punto
    void Encapsulate(const glm::vec3& point)
    {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    // Alias de Encapsulate (algunos lo llaman Expand)
    void Expand(const glm::vec3& point)
    {
        Encapsulate(point);
    }

    // Expandir AABB para incluir otra AABB
    void Encapsulate(const AABB& other)
    {
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
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

    // Obtener el tamaño de la diagonal
    float GetRadius() const
    {
        return glm::length(GetSize()) * 0.5f;
    }

    // Verificar si el AABB es válido
    bool IsValid() const
    {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    // Verificar si contiene un punto
    bool Contains(const glm::vec3& point) const
    {
        return point.x >= min.x && point.x <= max.x &&
            point.y >= min.y && point.y <= max.y &&
            point.z >= min.z && point.z <= max.z;
    }

    // Verificar si intersecta con otro AABB
    bool Intersects(const AABB& other) const
    {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
            (min.y <= other.max.y && max.y >= other.min.y) &&
            (min.z <= other.max.z && max.z >= other.min.z);
    }

    // Transformar AABB por una matriz
    AABB Transform(const glm::mat4& matrix) const
    {
        // Obtener las 8 esquinas del AABB
        glm::vec3 corners[8] = {
            glm::vec3(min.x, min.y, min.z),
            glm::vec3(min.x, min.y, max.z),
            glm::vec3(min.x, max.y, min.z),
            glm::vec3(min.x, max.y, max.z),
            glm::vec3(max.x, min.y, min.z),
            glm::vec3(max.x, min.y, max.z),
            glm::vec3(max.x, max.y, min.z),
            glm::vec3(max.x, max.y, max.z)
        };

        // Crear nuevo AABB transformado
        AABB result;
        for (int i = 0; i < 8; ++i)
        {
            glm::vec3 transformedCorner = glm::vec3(matrix * glm::vec4(corners[i], 1.0f));
            result.Encapsulate(transformedCorner);
        }

        return result;
    }

    // Obtener las 8 esquinas del AABB
    void GetCorners(glm::vec3 corners[8]) const
    {
        corners[0] = glm::vec3(min.x, min.y, min.z);
        corners[1] = glm::vec3(min.x, min.y, max.z);
        corners[2] = glm::vec3(min.x, max.y, min.z);
        corners[3] = glm::vec3(min.x, max.y, max.z);
        corners[4] = glm::vec3(max.x, min.y, min.z);
        corners[5] = glm::vec3(max.x, min.y, max.z);
        corners[6] = glm::vec3(max.x, max.y, min.z);
        corners[7] = glm::vec3(max.x, max.y, max.z);
    }
};