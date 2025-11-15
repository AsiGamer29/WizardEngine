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
    // Representación estructurada (para OpenGL y uso interno)
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;

    // Representación plana (para raycast - generada bajo demanda)
    mutable std::vector<float> flatVertices;
    mutable bool flatVerticesDirty = true;

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLuint numIndices, numVertices;

    AABB localAABB;
    bool aabbDirty = true;

    void SetupMesh();
    void CleanupBuffers();
    void UpdateFlatVertices() const;

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

    // GETTERS NECESARIOS PARA RAYCAST
    const std::vector<float>& GetVertices() const;
    const std::vector<MeshVertex>& GetMeshVertices() const { return vertices; }
    const std::vector<unsigned int>& GetIndices() const { return indices; }

    size_t GetVertexCount() const { return vertices.size(); }
    size_t GetIndexCount() const { return indices.size(); }

    // Sistema de AABB
    AABB CalculateLocalAABB() const;
    AABB GetLocalAABB();
};