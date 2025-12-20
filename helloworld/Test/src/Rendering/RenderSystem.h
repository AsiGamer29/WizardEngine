#pragma once
#include <vector>
#include <map>
#include "GameObject.h"
#include "ComponentMaterial.h"
#include "ComponentMesh.h"
#include "ComponentTransform.h"
#include "Frustum.h"
#include "Octree.h"
#include "DebugRenderer.h"
#include <glm/glm.hpp>

class ModuleScene;

class RenderSystem
{
public:
    struct RenderStats
    {
        int totalObjects = 0;
        int visibleObjects = 0;
        int culledObjects = 0;
        int octreeCandidates = 0;
    };

    static void RenderScene(const std::vector<GameObject*>& gameObjects, const glm::vec3& cameraPos, 
                          const Frustum* frustum = nullptr, bool enableFrustumCulling = true, 
                          bool drawDebugAABBs = false, ModuleScene* scene = nullptr);

    static const RenderStats& GetRenderStats() { return renderStats; }

private:
    static void RenderObject(GameObject* obj);

    static RenderStats renderStats;
};