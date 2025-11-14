#pragma once
#include "BaseComponent.h"
#include "GeometryGenerator.h"
#include "AABB.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <assimp/scene.h>

// Estructura de vértice para ComponentMesh
struct MeshVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

class ComponentMesh : public Component
{
private:
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;
    GLuint VAO, VBO, EBO, numIndices, numVertices;
    AABB localAABB;
    bool aabbDirty = true;

    void SetupMesh();
    void CleanupBuffers();

public:
    ComponentMesh(GameObject* owner);
    ~ComponentMesh();

    // Métodos heredados de Component
    void Update() override;
    void OnEditor() override;

    // Cargar mesh desde Assimp (para modelos FBX/OBJ)
    void LoadMesh(const aiMesh* mesh);

    // Cargar desde geometría procedural
    void LoadFromGeometry(MeshGeometry* geom);

    // Renderizar
    void Draw();

    // Debug: dibujar normales en pantalla
    void DrawNormals(const glm::mat4& modelMatrix, float length = 0.1f);

    // Getters
    size_t GetVertexCount() const { return vertices.size(); }
    size_t GetIndexCount() const { return indices.size(); }

    // CORREGIDO: Devolver referencias a vectores correctos
    const std::vector<MeshVertex>& GetVertices() const { return vertices; }
    const std::vector<unsigned int>& GetIndices() const { return indices; }

    // Sistema de AABB
    AABB CalculateLocalAABB() const;
    AABB GetLocalAABB();
};