#pragma once
#include "Module.h"
#include "Shader.h"
#include <string>
#include <glm/glm.hpp>

class Model;
class MeshGeometry;
class GameObject;

class OpenGL : public Module
{
private:
    void* glContext;
    Shader* shader;
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
    bool showGrid;

    // Geometría procedural
    MeshGeometry* currentGeometry;
    bool isGeometryActive;

    void CreateGrid(int size);
    void DrawGrid();

    // NUEVAS FUNCIONES PARA GAMEOBJECTS
    void DrawGameObjects(GameObject* go);
    void ApplyTextureToGameObjects(GameObject* go, GLuint texID, const char* path);

public:
    OpenGL();
    ~OpenGL();

    bool Start() override;
    bool PreUpdate() override;
    bool Update() override;
    bool CleanUp() override;

    void LoadGeometry(const std::string& type);

    // Getters para el editor
    bool IsGridVisible() const { return showGrid; }
    void SetGridVisible(bool visible) { showGrid = visible; }
};