#pragma once
#include "Module.h"
#include "Shader.h"
#include <SDL3/SDL.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct SDL_Window;
class Model; // Forward declaration

enum class ShapeType { Cube, Sphere, Cylinder, Pyramid };
static ShapeType currentShape = ShapeType::Cube;

class OpenGL : public Module
{
public:
    OpenGL();  // Declaración del constructor
    ~OpenGL();

    SDL_GLContext glContext;
    Shader* shader = nullptr;

    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
    unsigned int texture;

    glm::mat4 modelMatrix;  // Renombrado para evitar conflicto
    glm::mat4 view;
    glm::mat4 projection;
    float rotationAngle;

    Model* fbxModel = nullptr;

    void CreateGrid(int size);
    void DrawGrid();
    bool PreUpdate() override;

private:
    bool Start() override;
    bool CleanUp() override;
    void processInput(SDL_Window* window);
    bool Update() override;

    GLuint gridVAO = 0;
    GLuint gridVBO = 0;
    int gridLineCount = 0;
    bool showGrid = true;

    unsigned int sphereVAO = 0, sphereVBO = 0, sphereEBO = 0;
    unsigned int sphereIndexCount = 0;
    unsigned int pyramidVAO = 0, pyramidVBO = 0, pyramidEBO = 0;
    unsigned int pyramidIndexCount = 0;
    unsigned int cylinderVAO = 0, cylinderVBO = 0, cylinderEBO = 0;
    unsigned int cylinderIndexCount = 0;

};