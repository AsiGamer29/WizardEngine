#include "Octree.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include <algorithm>
#include <limits>

// Helper function para intersección ray-AABB
static bool IntersectRayAABB(const Ray& ray, const AABB& aabb)
{
    float tmin = -std::numeric_limits<float>::infinity();
    float tmax = std::numeric_limits<float>::infinity();

    for (int i = 0; i < 3; ++i)
    {
        float t1 = (aabb.min[i] - ray.origin[i]) / ray.direction[i];
        float t2 = (aabb.max[i] - ray.origin[i]) / ray.direction[i];

        tmin = std::max(tmin, std::min(t1, t2));
        tmax = std::min(tmax, std::max(t1, t2));
    }

    return tmax >= tmin && tmax >= 0.0f;
}

// ============================================================================
// OctreeNode Implementation
// ============================================================================

OctreeNode::OctreeNode(const AABB& bounds, int maxObjects, int maxDepth, int depth)
    : bounds(bounds)
    , maxObjectsPerNode(maxObjects)
    , maxDepth(maxDepth)
    , currentDepth(depth)
    , isDivided(false)
{
    objects.reserve(maxObjectsPerNode);
}

void OctreeNode::Insert(GameObject* gameObject, const AABB& worldAABB)
{
    if (!gameObject || !bounds.Intersects(worldAABB))
        return;

    // Si no está subdividido y no hemos alcanzado el límite, agregamos aquí
    if (!isDivided && objects.size() < static_cast<size_t>(maxObjectsPerNode))
    {
        objects.emplace_back(gameObject, worldAABB);
        return;
    }

    // Si debemos subdividir
    if (!isDivided && ShouldSubdivide())
    {
        Subdivide();

        // Redistribuir objetos existentes
        std::vector<OctreeObject> temp = std::move(objects);
        objects.clear();

        for (const auto& obj : temp)
        {
            Insert(obj.gameObject, obj.worldAABB);
        }
    }

    // Si está subdividido, intentar insertar en hijos
    if (isDivided)
    {
        bool inserted = false;
        for (int i = 0; i < 8; ++i)
        {
            if (children[i].get() && children[i]->GetBounds().Intersects(worldAABB))
            {
                children[i]->Insert(gameObject, worldAABB);
                inserted = true;
                break;
            }
        }

        // Si no cabe completamente en ningún hijo, guardarlo aquí
        if (!inserted)
        {
            objects.emplace_back(gameObject, worldAABB);
        }
    }
    else
    {
        // Si no se puede subdividir más, guardar aquí
        objects.emplace_back(gameObject, worldAABB);
    }
}

void OctreeNode::QueryFrustum(const Frustum& frustum, std::vector<GameObject*>& result) const
{
    // Early out si el nodo no intersecta con el frustum
    if (!frustum.IsBoxVisible(bounds))
        return;

    // Agregar objetos de este nodo
    for (const auto& obj : objects)
    {
        if (obj.gameObject && frustum.IsBoxVisible(obj.worldAABB))
        {
            result.push_back(obj.gameObject);
        }
    }

    // Recursivamente consultar hijos
    if (isDivided)
    {
        for (int i = 0; i < 8; ++i)
        {
            if (children[i])
            {
                children[i]->QueryFrustum(frustum, result);
            }
        }
    }
}

void OctreeNode::QueryRay(const Ray& ray, std::vector<GameObject*>& result) const
{
    // Early out si el rayo no intersecta el nodo
    if (!IntersectRayAABB(ray, bounds))
        return;

    // Comprobar objetos en este nodo
    for (const auto& obj : objects)
    {
        if (obj.gameObject && IntersectRayAABB(ray, obj.worldAABB))
        {
            result.push_back(obj.gameObject);
        }
    }

    // Recursivamente consultar hijos
    if (isDivided)
    {
        for (int i = 0; i < 8; ++i)
        {
            if (children[i])
            {
                children[i]->QueryRay(ray, result);
            }
        }
    }
}

void OctreeNode::Clear()
{
    objects.clear();
    
    if (isDivided)
    {
        for (int i = 0; i < 8; ++i)
        {
            if (children[i])
            {
                children[i]->Clear();
                children[i].reset();
            }
        }
        isDivided = false;
    }
}

void OctreeNode::Rebuild(const std::vector<GameObject*>& allObjects)
{
    Clear();
    
    for (GameObject* obj : allObjects)
    {
        if (!obj || !obj->HasAABB())
            continue;

        ComponentTransform* transform = obj->GetComponent<ComponentTransform>();
        AABB worldAABB = obj->GetAABB();
        
        if (transform)
        {
            worldAABB = worldAABB.Transform(transform->GetGlobalMatrix());
        }

        Insert(obj, worldAABB);
    }
}

void OctreeNode::Subdivide()
{
    if (isDivided || currentDepth >= maxDepth)
        return;

    glm::vec3 center = bounds.GetCenter();
    glm::vec3 halfSize = bounds.GetSize() * 0.5f;

    // Crear 8 hijos
    // Orden: -x-y-z, +x-y-z, -x+y-z, +x+y-z, -x-y+z, +x-y+z, -x+y+z, +x+y+z
    glm::vec3 offsets[8] = {
        glm::vec3(-1, -1, -1), glm::vec3(+1, -1, -1),
        glm::vec3(-1, +1, -1), glm::vec3(+1, +1, -1),
        glm::vec3(-1, -1, +1), glm::vec3(+1, -1, +1),
        glm::vec3(-1, +1, +1), glm::vec3(+1, +1, +1)
    };

    for (int i = 0; i < 8; ++i)
    {
        glm::vec3 childCenter = center + offsets[i] * halfSize * 0.5f;
        glm::vec3 childHalfSize = halfSize * 0.5f;
        
        AABB childBounds(childCenter - childHalfSize, childCenter + childHalfSize);
        children[i] = std::make_unique<OctreeNode>(
            childBounds, 
            maxObjectsPerNode, 
            maxDepth, 
            currentDepth + 1
        );
    }

    isDivided = true;
}

bool OctreeNode::ShouldSubdivide() const
{
    return currentDepth < maxDepth && 
           objects.size() > static_cast<size_t>(maxObjectsPerNode);
}

void OctreeNode::GetAllBounds(std::vector<AABB>& outBounds) const
{
    outBounds.push_back(bounds);
    
    if (isDivided)
    {
        for (int i = 0; i < 8; ++i)
        {
            if (children[i])
            {
                children[i]->GetAllBounds(outBounds);
            }
        }
    }
}

int OctreeNode::GetObjectCount() const
{
    int count = static_cast<int>(objects.size());
    
    if (isDivided)
    {
        for (int i = 0; i < 8; ++i)
        {
            if (children[i])
            {
                count += children[i]->GetObjectCount();
            }
        }
    }
    
    return count;
}

// ============================================================================
// Octree Implementation
// ============================================================================

Octree::Octree(const AABB& worldBounds, int maxObjects, int maxDepth)
    : worldBounds(worldBounds)
    , maxObjectsPerNode(maxObjects)
    , maxDepth(maxDepth)
    , needsRebuild(false)
{
    root = std::make_unique<OctreeNode>(worldBounds, maxObjects, maxDepth, 0);
}

void Octree::Build(const std::vector<GameObject*>& allObjects)
{
    // Calcular bounds del mundo basado en los objetos
    if (!allObjects.empty())
    {
        worldBounds = CalculateWorldBounds(allObjects);
    }

    // Crear nodo raíz
    root = std::make_unique<OctreeNode>(worldBounds, maxObjectsPerNode, maxDepth, 0);

    // Insertar todos los objetos
    for (GameObject* obj : allObjects)
    {
        if (!obj || !obj->HasAABB())
            continue;

        ComponentTransform* transform = obj->GetComponent<ComponentTransform>();
        AABB worldAABB = obj->GetAABB();
        
        if (transform)
        {
            worldAABB = worldAABB.Transform(transform->GetGlobalMatrix());
        }

        root->Insert(obj, worldAABB);
    }

    needsRebuild = false;
}

void Octree::Rebuild()
{
    // Esta función requiere acceso a todos los objetos
    // Debería ser llamada por ModuleScene
    Clear();
    needsRebuild = true;
}

void Octree::Insert(GameObject* gameObject, const AABB& worldAABB)
{
    if (!root)
    {
        root = std::make_unique<OctreeNode>(worldBounds, maxObjectsPerNode, maxDepth, 0);
    }

    root->Insert(gameObject, worldAABB);
}

void Octree::QueryFrustum(const Frustum& frustum, std::vector<GameObject*>& result) const
{
    if (root)
    {
        root->QueryFrustum(frustum, result);
    }
}

void Octree::QueryRay(const Ray& ray, std::vector<GameObject*>& result) const
{
    if (root)
    {
        root->QueryRay(ray, result);
    }
}

void Octree::Clear()
{
    if (root)
    {
        root->Clear();
    }
}

void Octree::GetAllBounds(std::vector<AABB>& outBounds) const
{
    if (root)
    {
        root->GetAllBounds(outBounds);
    }
}

int Octree::GetTotalObjects() const
{
    return root ? root->GetObjectCount() : 0;
}

void Octree::SetWorldBounds(const AABB& bounds)
{
    worldBounds = bounds;
    needsRebuild = true;
}

AABB Octree::CalculateWorldBounds(const std::vector<GameObject*>& objects) const
{
    if (objects.empty())
    {
        return AABB(glm::vec3(-100.0f), glm::vec3(100.0f));
    }

    glm::vec3 minBound(std::numeric_limits<float>::max());
    glm::vec3 maxBound(std::numeric_limits<float>::lowest());

    for (GameObject* obj : objects)
    {
        if (!obj || !obj->HasAABB())
            continue;

        ComponentTransform* transform = obj->GetComponent<ComponentTransform>();
        AABB worldAABB = obj->GetAABB();
        
        if (transform)
        {
            worldAABB = worldAABB.Transform(transform->GetGlobalMatrix());
        }

        minBound = glm::min(minBound, worldAABB.min);
        maxBound = glm::max(maxBound, worldAABB.max);
    }

    // Añadir padding del 10%
    glm::vec3 size = maxBound - minBound;
    glm::vec3 padding = size * 0.1f;
    
    return AABB(minBound - padding, maxBound + padding);
}
