#pragma once
#include <vector>
#include <map>
#include "GameObject.h"
#include "ComponentMaterial.h"
#include "ComponentMesh.h"
#include "ComponentTransform.h"
#include <glm/glm.hpp>

class RenderSystem
{
public:
    static void RenderScene(const std::vector<GameObject*>& gameObjects, const glm::vec3& cameraPos)
    {
        std::vector<GameObject*> opaqueObjects;
        std::vector<GameObject*> alphaTestObjects;
        std::map<float, GameObject*, std::greater<float>> blendedObjects;

        for (GameObject* obj : gameObjects)
        {
            if (!obj->IsActive()) continue;

            ComponentMaterial* mat = obj->GetComponent<ComponentMaterial>();
            ComponentMesh* mesh = obj->GetComponent<ComponentMesh>();

            if (!mat || !mesh) continue;

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
                ComponentTransform* trans = obj->GetComponent<ComponentTransform>();
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

private:
    static void RenderObject(GameObject* obj)
    {
        ComponentMaterial* mat = obj->GetComponent<ComponentMaterial>();
        ComponentMesh* mesh = obj->GetComponent<ComponentMesh>();

        if (mat) mat->Bind();
        if (mesh) mesh->Draw();
        if (mat) mat->Unbind();
    }
};