#pragma once
#include "Module.h"
#include "Shader.h"
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <glm/glm.hpp>
#include "AABB.h" 

class Model;
class MeshGeometry;
class GameObject;

class OpenGL : public Module
{
private:
    void* glContext;
    Shader* shader;
    Shader* debugShader; // shader para debug (normales)
    Model* fbxModel;

    glm::mat4 modelMatrix;
    glm::mat4 view;
    glm::mat4 projection;
    float rotationAngle;

    unsigned int texture;

    // Grid
    unsigned int gridVAO;
    unsigned int gridVBO;
    int gridLineCount;

    // Geometría procedural
    MeshGeometry* currentGeometry;
    bool isGeometryActive;

    void CreateGrid(int size);
  

    // NUEVAS FUNCIONES PARA GAMEOBJECTS
    void DrawGameObjects(GameObject* go);
    void ApplyTextureToGameObjects(GameObject* go, GLuint texID, const char* path);

    GLuint aabbVAO = 0;
    GLuint aabbVBO = 0;
    void CreateAABBBuffers();

    GLuint sceneFBO, sceneTexture, sceneRBO;
    int sceneWidth = 1280, sceneHeight = 720;

public:
    OpenGL();
    ~OpenGL();

    bool Start() override;
    bool PreUpdate() override;
    bool Update() override;
    bool CleanUp() override;

    void DrawGrid();

    void LoadGeometry(const std::string& type);

    void CollectTexturesInUse(GameObject* go, std::set<GLuint>& texturesInUse);

    // Variables públicas para debug
    bool showAABBs = false;
    bool showGrid = true;

    // Getters para el editor
    bool IsGridVisible() const { return showGrid; }
    void SetGridVisible(bool visible) { showGrid = visible; }
    Shader* gridShader;
    glm::vec3 gridColor = glm::vec3(0.5f, 0.5f, 0.5f);

    // Métodos de visualización AABB
    void DrawAABB(const AABB& aabb, const glm::vec3& color = glm::vec3(0.0f, 1.0f, 0.0f));
    void DrawGameObjectsWithAABB(GameObject* go);
};