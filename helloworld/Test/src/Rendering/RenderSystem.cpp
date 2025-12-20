#include "RenderSystem.h"
#include "ModuleScene.h"
#include "OpenGL.h"

RenderSystem::RenderStats RenderSystem::renderStats;

void RenderSystem::RenderScene(const std::vector<GameObject*>& gameObjects, const glm::vec3& cameraPos,
    const Frustum* frustum, bool enableFrustumCulling,
    bool drawDebugAABBs, ModuleScene* scene)
{
    renderStats.totalObjects = 0;
    renderStats.visibleObjects = 0;
    renderStats.culledObjects = 0;
    renderStats.octreeCandidates = 0;

    std::vector<GameObject*> opaqueObjects;
    std::vector<GameObject*> alphaTestObjects;
    std::map<float, GameObject*, std::greater<float>> blendedObjects;

    // Determinar qué objetos procesar
    std::vector<GameObject*> objectsToProcess;

    // Intentar usar Octree si está disponible
    if (enableFrustumCulling && frustum && scene && scene->IsUsingOctree() && scene->GetOctree())
    {
        // Query optimizada con Octree
        scene->GetOctree()->QueryFrustum(*frustum, objectsToProcess);
        renderStats.octreeCandidates = static_cast<int>(objectsToProcess.size());
    }
    else
    {
        // Fallback: usar todos los objetos
        objectsToProcess = gameObjects;
    }

    for (GameObject* obj : objectsToProcess)
    {
        if (!obj->IsActive()) continue;

        ComponentMaterial* mat = obj->GetComponent<ComponentMaterial>();
        ComponentMesh* mesh = obj->GetComponent<ComponentMesh>();
        ComponentTransform* trans = obj->GetComponent<ComponentTransform>();

        if (!mat || !mesh) continue;

        renderStats.totalObjects++;

        // Frustum culling refinado (si usamos Octree, la mayoría ya pasó el test)
        bool isVisible = true;
        if (enableFrustumCulling && frustum && obj->HasAABB())
        {
            AABB worldAABB = obj->GetAABB();
            if (trans)
            {
                worldAABB = worldAABB.Transform(trans->GetGlobalMatrix());
            }

            isVisible = frustum->IsBoxVisible(worldAABB);

            if (!isVisible)
            {
                renderStats.culledObjects++;
            }

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

void RenderSystem::RenderObject(GameObject* obj)
{
    ComponentMaterial* mat = obj->GetComponent<ComponentMaterial>();
    ComponentMesh* mesh = obj->GetComponent<ComponentMesh>();

    if (mat) mat->Bind();
    if (mesh) mesh->Draw();
    if (mat) mat->Unbind();
}

