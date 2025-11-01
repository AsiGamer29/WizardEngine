#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

struct GeomVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct MeshGeometry {
    std::vector<GeomVertex> vertices;
    std::vector<unsigned int> indices;
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;

    void SetupMesh();
    void Draw();
    void Cleanup();
};

class GeometryGenerator {
public:
    static MeshGeometry CreateCube(float size = 1.0f);
    static MeshGeometry CreateSphere(float radius = 1.0f, int segments = 32, int rings = 16);
    static MeshGeometry CreateCylinder(float radius = 1.0f, float height = 2.0f, int segments = 32);
    static MeshGeometry CreatePyramid(float base = 1.0f, float height = 1.0f);
    static MeshGeometry CreatePlane(float width = 10.0f, float depth = 10.0f);
};