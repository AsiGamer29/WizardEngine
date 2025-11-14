#include "AABB.h"
#include "Ray.h"
#include <algorithm>
#include <cmath>

bool AABB::IntersectRay(const Ray& ray, float& tMin, float& tMax) const
{
    if (!IsValid())
        return false;

    // Algoritmo "Slab Method" para intersección rayo-AABB
    // Es muy eficiente y robusto
    tMin = 0.0f;
    tMax = FLT_MAX;

    // Probar cada eje (X, Y, Z)
    for (int i = 0; i < 3; ++i)
    {
        // Si el rayo es paralelo a este plano
        if (std::abs(ray.direction[i]) < 1e-8f)
        {
            // Si el origen del rayo está fuera del slab, no hay intersección
            if (ray.origin[i] < min[i] || ray.origin[i] > max[i])
                return false;
        }
        else
        {
            // Calcular distancias t a los planos del slab
            float invD = 1.0f / ray.direction[i];
            float t1 = (min[i] - ray.origin[i]) * invD;
            float t2 = (max[i] - ray.origin[i]) * invD;

            // Asegurar que t1 <= t2
            if (t1 > t2)
                std::swap(t1, t2);

            // Actualizar tMin y tMax
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);

            // Si tMin > tMax, no hay intersección
            if (tMin > tMax)
                return false;
        }
    }

    // Si llegamos aquí, hay intersección
    // tMin es la distancia al punto de entrada
    // tMax es la distancia al punto de salida
    return true;
}

bool AABB::IntersectRay(const Ray& ray) const
{
    float tMin, tMax;
    return IntersectRay(ray, tMin, tMax);
}