#pragma once
#include <vector>
#include <map>
#include "GameObject.h"
#include "ComponentMaterial.h"
#include "ComponentMesh.h"
#include "ComponentTransform.h"
#include "ShaderManager.h"
#include "Shader.h"
#include "Application.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
            RenderObject(obj, cameraPos);
        }

        for (GameObject* obj : alphaTestObjects)
        {
            RenderObject(obj, cameraPos);
        }

        glDepthMask(GL_FALSE);
        for (auto& pair : blendedObjects)
        {
            RenderObject(pair.second, cameraPos);
        }
        glDepthMask(GL_TRUE);
    }

private:
    static void RenderObject(GameObject* obj, const glm::vec3& cameraPos)
    {
        ComponentMaterial* mat = obj->GetComponent<ComponentMaterial>();
        ComponentMesh* mesh = obj->GetComponent<ComponentMesh>();
        ComponentTransform* trans = obj->GetComponent<ComponentTransform>();

        if (!mat || !mesh || !trans) return;

        // SELECCIONAR SHADER SEGUN EL TIPO
        Shader* shader = nullptr;
        ShaderType shaderType = mat->GetShaderType();

        if (shaderType == ShaderType::UNLIT)
        {
            shader = ShaderManager::GetUnlitShader();
        }
        else
        {
            shader = ShaderManager::GetPhongShader();
        }

        if (!shader) return;

        shader->use();

        // MATRICES
        glm::mat4 model = trans->GetGlobalMatrix();
        auto& app = Application::GetInstance();
        glm::mat4 view = app.camera->getViewMatrix();
        glm::mat4 projection = app.camera->getProjectionMatrix();

        shader->setMat4("model", model);
        shader->setMat4("view", view);
        shader->setMat4("projection", projection);

        // ALPHA TEST
        bool useAlphaTest = (mat->GetAlphaMode() == AlphaMode::ALPHA_TEST);
        shader->setBool("useAlphaTest", useAlphaTest);
        shader->setFloat("alphaCutoff", mat->GetAlphaCutoff());

        // COLOR TINT
        shader->setVec4("colorTint", mat->GetColorTint());

        // TEXTURE
        shader->setInt("texture1", 0);
        glActiveTexture(GL_TEXTURE0);

        // SI USA LIGHTING (NO UNLIT)
        if (shaderType != ShaderType::UNLIT)
        {
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
            shader->setMat3("normalMatrix", normalMatrix);

            // LIGHT DIRECTION (luz direccional desde arriba)
            glm::vec3 lightDir = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f));
            glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);

            shader->setVec3("lightDir", lightDir);
            shader->setVec3("lightColor", lightColor);
            shader->setVec3("viewPos", cameraPos);

            // MATERIAL PROPERTIES
            shader->setVec3("ambientColor", mat->GetAmbientColor());
            shader->setVec3("diffuseColor", mat->GetDiffuseColor());
            shader->setVec3("specularColor", mat->GetSpecularColor());
            shader->setFloat("shininess", mat->GetShininess());

            // LIGHTING MODE
            bool doVertexLighting = (shaderType == ShaderType::VERTEX_PHONG);
            bool useBlinnPhong = (shaderType == ShaderType::PIXEL_BLINN_PHONG);

            shader->setBool("doVertexLighting", doVertexLighting);
            shader->setBool("useBlinnPhong", useBlinnPhong);
        }

        mat->Bind();
        mesh->Draw();
        mat->Unbind();
    }
};