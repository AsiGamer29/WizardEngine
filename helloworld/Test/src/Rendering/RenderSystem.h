#pragma once
#include <vector>
#include <map>
#include "GameObject.h"
#include "ComponentMaterial.h"
#include "ComponentMesh.h"
#include "ComponentTransform.h"
#include "Frustum.h"
#include "DebugRenderer.h"
#include <glm/glm.hpp>

class RenderSystem
{
public:
    struct RenderStats
    {
        int totalObjects = 0;
        int visibleObjects = 0;
        int culledObjects = 0;
    };

    static void RenderScene(const std::vector<GameObject*>& gameObjects, const glm::vec3& cameraPos, 
                          const Frustum* frustum = nullptr, bool enableFrustumCulling = true, 
                          bool drawDebugAABBs = false)
    {
        renderStats.totalObjects = 0;
        renderStats.visibleObjects = 0;
        renderStats.culledObjects = 0;

        std::vector<GameObject*> opaqueObjects;
        std::vector<GameObject*> alphaTestObjects;
        std::map<float, GameObject*, std::greater<float>> blendedObjects;

        for (GameObject* obj : gameObjects)
        {
            if (!obj->IsActive()) continue;

            ComponentMaterial* mat = obj->GetComponent<ComponentMaterial>();
            ComponentMesh* mesh = obj->GetComponent<ComponentMesh>();
            ComponentTransform* trans = obj->GetComponent<ComponentTransform>();

            if (!mat || !mesh) continue;

            renderStats.totalObjects++;

            // Frustum culling
            bool isVisible = true;
            if (enableFrustumCulling && frustum && obj->HasAABB())
            {
                AABB worldAABB = obj->GetAABB();
                if (trans)
                {
                    // Transform AABB to world space
                    worldAABB = worldAABB.Transform(trans->GetGlobalMatrix());
                }
                
                isVisible = frustum->IsBoxVisible(worldAABB);
                
                if (!isVisible)
                {
                    renderStats.culledObjects++;
                }

                // Debug visualization
                if (drawDebugAABBs)
                {
                    glm::vec3 color = isVisible ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
                    DebugRenderer::Get().DrawAABB(worldAABB, color);
                }
            }

            if (!isVisible)
                continue;

            renderStats.visibleObjects++;

            if (mat->IsOpaque())
            {
                opaqueObjects.push_back(obj);
            }
            else if (mat->GetAlphaMode() == AlphaMode::ALPHA_TEST)
            {
                alphaTestObjects.push_back(obj);
            }
            else if (mat->NeedsBlending())
            {
                if (trans)
                {
                    glm::vec3 pos = trans->GetPosition();
                    float dist = glm::distance(cameraPos, pos);
                    blendedObjects[dist] = obj;
                }
            }
        }

        glDepthMask(GL_TRUE);
        for (GameObject* obj : opaqueObjects)
        {
            RenderObject(obj);
        }

        for (GameObject* obj : alphaTestObjects)
        {
            RenderObject(obj);
        }

        glDepthMask(GL_FALSE);
        for (auto& pair : blendedObjects)
        {
            RenderObject(pair.second);
        }
        glDepthMask(GL_TRUE);
    }

    static const RenderStats& GetRenderStats() { return renderStats; }

private:
    static void RenderObject(GameObject* obj)
    {
        ComponentMaterial* mat = obj->GetComponent<ComponentMaterial>();
        ComponentMesh* mesh = obj->GetComponent<ComponentMesh>();

        if (mat) mat->Bind();
        if (mesh) mesh->Draw();
        if (mat) mat->Unbind();
    }

    static RenderStats renderStats;
};