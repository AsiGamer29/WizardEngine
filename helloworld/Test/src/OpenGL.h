#pragma once
#include "Module.h"
#include "Shader.h"
#include "Model.h"
#include "Camera.h"
#include "GeometryGenerator.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <string>

class OpenGL : public Module
{
public:
    OpenGL();
    ~OpenGL();

    bool Start() override;
    bool PreUpdate() override;
    bool Update() override;
    bool CleanUp() override;

    void CreateGrid(int size);
    void DrawGrid();
    void LoadGeometry(const std::string& type);

    // Grid
    GLuint gridVAO = 0;
    GLuint gridVBO = 0;
    int gridLineCount = 0;
    bool showGrid = true;

private:
    SDL_GLContext glContext;
    Shader* shader;
    Model* fbxModel;

    MeshGeometry* currentGeometry = nullptr;
    bool isGeometryActive = false;

    GLuint texture;
    float rotationAngle;

    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 modelMatrix;
};