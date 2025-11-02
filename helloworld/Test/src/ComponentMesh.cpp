#include "ComponentMesh.h"
#include "GameObject.h"
#include <glad/glad.h>
#include <iostream>

ComponentMesh::ComponentMesh(GameObject* owner)
    :Component(owner, ComponentType::MESH),
    VAO(0), VBO(0), EBO(0), numVertices(0), numIndices(0)
{
}

ComponentMesh::~ComponentMesh()
{
    CleanupBuffers();
}

void ComponentMesh::LoadMesh(const aiMesh* mesh)
{
    if (!mesh)
    {
        std::cerr << "[ComponentMesh] Invalid mesh pointer" << std::endl;
        return;
    }

    // Limpiar datos anteriores
    CleanupBuffers();
    vertices.clear();
    indices.clear();

    // Cargar vértices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        MeshVertex vertex;

        // Posición
        vertex.Position.x = mesh->mVertices[i].x;
        vertex.Position.y = mesh->mVertices[i].y;
        vertex.Position.z = mesh->mVertices[i].z;

        // Normales
        if (mesh->HasNormals())
        {
            vertex.Normal.x = mesh->mNormals[i].x;
            vertex.Normal.y = mesh->mNormals[i].y;
            vertex.Normal.z = mesh->mNormals[i].z;
        }
        else
        {
           vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        // Coordenadas de textura
        if (mesh->mTextureCoords[0])
        {
            vertex.TexCoords.x = mesh->mTextureCoords[0][i].x;
            vertex.TexCoords.y = mesh->mTextureCoords[0][i].y;
        }
        else
        {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    // Cargar índices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    numVertices = vertices.size();
    numIndices = indices.size();

    // Configurar buffers de OpenGL
    SetupMesh();

    std::cout << "[ComponentMesh] Loaded mesh: "
        << numVertices << " vertices, "
        << numIndices << " indices" << std::endl;
}

void ComponentMesh::SetupMesh()
{
    // Generar buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // VBO - Vertex Buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(MeshVertex), &vertices[0], GL_STATIC_DRAW);

    // EBO - Element Buffer (índices)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // Atributo 0: Posición
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)0);

    // Atributo 1: Normales
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, Normal));

    // Atributo 2: Coordenadas de textura
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, TexCoords));

    glBindVertexArray(0);
}

void ComponentMesh::Draw()
{
    if (VAO == 0 || numIndices == 0)
        return;

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void ComponentMesh::OnEditor()
{
    // TODO: Implementar con ImGui
    // ImGui::Text("Vertices: %d", numVertices);
    // ImGui::Text("Indices: %d", numIndices);
    // ImGui::Text("Triangles: %d", numIndices / 3);
}

void ComponentMesh::CleanupBuffers()
{
    if (EBO != 0)
    {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }

    if (VBO != 0)
    {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }

    if (VAO != 0)
    {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
}
void ComponentMesh::Update()
{
    // Si no necesitas actualizar nada en cada frame, déjalo vacío
    // Pero la implementación DEBE existir porque está declarada en el .h
}

void ComponentMesh::LoadFromGeometry(MeshGeometry* geom)
{
    if (!geom)
    {
        std::cerr << "[ComponentMesh] Invalid geometry pointer" << std::endl;
        return;
    }

    // Limpiar datos anteriores
    CleanupBuffers();
    vertices.clear();
    indices.clear();

    // Convertir de MeshGeometry a MeshVertex
    for (const auto& v : geom->vertices)
    {
        MeshVertex vertex;
        vertex.Position = v.Position;
        vertex.Normal = v.Normal;
        vertex.TexCoords = v.TexCoords;
        vertex.Tangent = glm::vec3(0.0f);    // O calcular si es necesario
        vertex.Bitangent = glm::vec3(0.0f);  // O calcular si es necesario

        vertices.push_back(vertex);
    }

    // Copiar índices
    indices = geom->indices;

    numVertices = vertices.size();
    numIndices = indices.size();

    // Configurar buffers de OpenGL
    SetupMesh();

    std::cout << "[ComponentMesh] Loaded procedural geometry: "
        << numVertices << " vertices, "
        << numIndices << " indices" << std::endl;
}