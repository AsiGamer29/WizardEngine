#pragma once
#include "Module.h"
#include "Shader.h"
#include <SDL3/SDL.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct SDL_Window;

enum class ShapeType { Cube, Sphere, Cylinder, Pyramid };
static ShapeType currentShape = ShapeType::Cube;

class OpenGL : public Module
{
public:
    OpenGL();
    ~OpenGL();

    SDL_GLContext glContext;
    Shader* shader = nullptr;
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
    unsigned int texture;

    // Matrices de transformación
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    float rotationAngle;  // Para la rotación de los objetos

private:
    bool Start() override;
    bool CleanUp() override;
    void processInput(SDL_Window* window);
    bool Update() override;

    // Objetos adicionales
    unsigned int sphereVAO = 0, sphereVBO = 0, sphereEBO = 0;
    unsigned int sphereIndexCount = 0;
    unsigned int pyramidVAO = 0, pyramidVBO = 0, pyramidEBO = 0;
    unsigned int pyramidIndexCount = 0;
    unsigned int cylinderVAO = 0, cylinderVBO = 0, cylinderEBO = 0;
    unsigned int cylinderIndexCount = 0;

    // Generadores de geometría
    void GenerateSphere(float radius, unsigned int stacks, unsigned int slices);
    void GeneratePyramid(float baseSize, float height);
    void GenerateCylinder(float radius, float height, int sectors);

    // Carga de textura
    unsigned int LoadTexture(const char* path);
};
