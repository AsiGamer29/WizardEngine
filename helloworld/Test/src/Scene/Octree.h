#pragma once
#include "AABB.h"
#include "Ray.h"
#include "Frustum.h"
#include <vector>
#include <memory>

class GameObject;

class OctreeNode
{
public:
    struct OctreeObject
    {
        GameObject* gameObject;
        AABB worldAABB;

        OctreeObject(GameObject* go, const AABB& aabb) 
            : gameObject(go), worldAABB(aabb) {}
    };

private:
    AABB bounds;
    std::vector<OctreeObject> objects;
    std::unique_ptr<OctreeNode> children[8];
    
    int maxObjectsPerNode;
    int maxDepth;
    int currentDepth;
    bool isDivided;

public:
    OctreeNode(const AABB& bounds, int maxObjects = 8, int maxDepth = 8, int depth = 0);
    ~OctreeNode() = default;

    // Insertar un objeto en el octree
    void Insert(GameObject* gameObject, const AABB& worldAABB);

    // Obtener objetos dentro del frustum (para culling)
    void QueryFrustum(const Frustum& frustum, std::vector<GameObject*>& result) const;

    // Obtener objetos que intersectan con un rayo (para picking)
    void QueryRay(const Ray& ray, std::vector<GameObject*>& result) const;

    // Limpiar el octree
    void Clear();

    // Reconstruir el octree completo
    void Rebuild(const std::vector<GameObject*>& allObjects);

    // Debug
    const AABB& GetBounds() const { return bounds; }
    void GetAllBounds(std::vector<AABB>& outBounds) const;
    int GetObjectCount() const;

private:
    void Subdivide();
    int GetOctantIndex(const AABB& objectAABB) const;
    bool ShouldSubdivide() const;
};

class Octree
{
private:
    std::unique_ptr<OctreeNode> root;
    AABB worldBounds;
    int maxObjectsPerNode;
    int maxDepth;
    bool needsRebuild;

public:
    Octree(const AABB& worldBounds = AABB(glm::vec3(-1000.0f), glm::vec3(1000.0f)), 
           int maxObjects = 8, 
           int maxDepth = 8);
    ~Octree() = default;

    // Construir/reconstruir el octree
    void Build(const std::vector<GameObject*>& allObjects);
    void Rebuild();

    // Insertar un objeto individual
    void Insert(GameObject* gameObject, const AABB& worldAABB);

    // Queries
    void QueryFrustum(const Frustum& frustum, std::vector<GameObject*>& result) const;
    void QueryRay(const Ray& ray, std::vector<GameObject*>& result) const;

    // Limpiar
    void Clear();

    // Marcar para reconstrucción
    void MarkForRebuild() { needsRebuild = true; }
    bool NeedsRebuild() const { return needsRebuild; }

    // Debug
    void GetAllBounds(std::vector<AABB>& outBounds) const;
    int GetTotalObjects() const;

    // Actualizar bounds del mundo
    void SetWorldBounds(const AABB& bounds);
    const AABB& GetWorldBounds() const { return worldBounds; }

private:
    AABB CalculateWorldBounds(const std::vector<GameObject*>& objects) const;
};
