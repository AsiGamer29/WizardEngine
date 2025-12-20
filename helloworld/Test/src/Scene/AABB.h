#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Forward declaration
struct Ray;

struct AABB
{
    glm::vec3 min;
    glm::vec3 max;

    // Constructores
    AABB();
    AABB(const glm::vec3& min, const glm::vec3& max);

    // Validación
    bool IsValid() const;
    void Reset();

    // Expansión
    void ExpandToInclude(const glm::vec3& point);
    void ExpandToInclude(const AABB& other);

    // Alias para compatibilidad
    void Encapsulate(const glm::vec3& point);
    void Encapsulate(const AABB& other);

    // Propiedades
    glm::vec3 GetCenter() const;
    glm::vec3 GetSize() const;
    glm::vec3 GetHalfSize() const;
    float GetVolume() const;

    // Pruebas de intersección
    bool Contains(const glm::vec3& point) const;
    bool Intersects(const AABB& other) const;
    bool IntersectsRay(const Ray& ray, float& tMin, float& tMax) const;

    // Transformación
    void GetCorners(glm::vec3* corners) const;
    AABB Transform(const glm::mat4& transform) const;

    // Utilidades
    static AABB Merge(const AABB& a, const AABB& b);
    float DistanceToPoint(const glm::vec3& point) const;
    glm::vec3 ClosestPoint(const glm::vec3& point) const;
};