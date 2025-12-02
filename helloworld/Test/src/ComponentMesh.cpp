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

// NUEVO: Dibujar normales como líneas. Crea un VBO temporal con pares (pos, pos+normal*length)
void ComponentMesh::DrawNormals(const glm::mat4& modelMatrix, float length)
{
    if (vertices.empty()) return;

    std::vector<float> lines;
    lines.reserve(vertices.size() * 6);

    for (const auto& v : vertices)
    {
        glm::vec3 p = v.Position;
        glm::vec3 n = v.Normal;
        glm::vec3 p2 = p + n * length;

        // P1
        lines.push_back(p.x);
        lines.push_back(p.y);
        lines.push_back(p.z);
        // P2
        lines.push_back(p2.x);
        lines.push_back(p2.y);
        lines.push_back(p2.z);
    }

    // Crear buffers temporales
    GLuint tmpVAO = 0, tmpVBO = 0;
    glGenVertexArrays(1, &tmpVAO);
    glGenBuffers(1, &tmpVBO);

    glBindVertexArray(tmpVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tmpVBO);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(float), lines.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // Usar estado fijo: color blanco para las normales y dibujar líneas
    glDisable(GL_TEXTURE_2D);
    glLineWidth(1.0f);
    // Asumimos que se usa un shader ya activo que respeta la matriz model/view/projection
    glBindVertexArray(tmpVAO);
    glDrawArrays(GL_LINES, 0, (GLsizei)(lines.size() / 3));
    glBindVertexArray(0);

    // Limpiar
    glDeleteBuffers(1, &tmpVBO);
    glDeleteVertexArrays(1, &tmpVAO);
}

void ComponentMesh::OnEditor()
{
    // TODO: Implementar con ImGui
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
    CalculateAABB();

    std::cout << "[ComponentMesh] Loaded procedural geometry: "
        << numVertices << " vertices, "
        << numIndices << " indices" << std::endl;

    aabbDirty = true;

}

AABB ComponentMesh::CalculateLocalAABB() const
{
    AABB aabb;

    if (vertices.empty())
        return aabb;

    // Iterar sobre todos los vértices usando la estructura MeshVertex
    for (const MeshVertex& vertex : vertices)
    {
        aabb.Encapsulate(vertex.Position);
    }

    return aabb;
}

AABB ComponentMesh::GetLocalAABB()
{
    if (aabbDirty)
    {
        localAABB = CalculateLocalAABB();
        aabbDirty = false;
    }
    return localAABB;
}

void ComponentMesh::UpdateFlatVertices() const
{
    if (!flatVerticesDirty)
        return;

    flatVertices.clear();
    flatVertices.reserve(vertices.size() * 8);

    // Formato: [x, y, z, nx, ny, nz, u, v]
    for (const MeshVertex& v : vertices)
    {
        flatVertices.push_back(v.Position.x);
        flatVertices.push_back(v.Position.y);
        flatVertices.push_back(v.Position.z);
        flatVertices.push_back(v.Normal.x);
        flatVertices.push_back(v.Normal.y);
        flatVertices.push_back(v.Normal.z);
        flatVertices.push_back(v.TexCoords.x);
        flatVertices.push_back(v.TexCoords.y);
    }

    flatVerticesDirty = false;
}

const std::vector<float>& ComponentMesh::GetVertices() const
{
    UpdateFlatVertices();
    return flatVertices;
}

nlohmann::json ComponentMesh::SerializeMesh() const
{
    nlohmann::json meshData;

    // Guardar vertices
    nlohmann::json verticesArray = nlohmann::json::array();
    for (const auto& vertex : vertices)
    {
        nlohmann::json v;
        v["Position"] = { vertex.Position.x, vertex.Position.y, vertex.Position.z };
        v["Normal"] = { vertex.Normal.x, vertex.Normal.y, vertex.Normal.z };
        v["TexCoords"] = { vertex.TexCoords.x, vertex.TexCoords.y };
        v["Tangent"] = { vertex.Tangent.x, vertex.Tangent.y, vertex.Tangent.z };
        v["Bitangent"] = { vertex.Bitangent.x, vertex.Bitangent.y, vertex.Bitangent.z };
        verticesArray.push_back(v);
    }
    meshData["Vertices"] = verticesArray;

    // Guardar indices
    nlohmann::json indicesArray = nlohmann::json::array();
    for (unsigned int index : indices)
    {
        indicesArray.push_back(index);
    }
    meshData["Indices"] = indicesArray;

    // Guardar AABB
    AABB aabb = localAABB;
    meshData["AABB_Min"] = { aabb.min.x, aabb.min.y, aabb.min.z };
    meshData["AABB_Max"] = { aabb.max.x, aabb.max.y, aabb.max.z };

    return meshData;
}

void ComponentMesh::DeserializeMesh(const nlohmann::json& meshData)
{
    if (!meshData.contains("Vertices") || !meshData.contains("Indices"))
        return;

    vertices.clear();
    indices.clear();

    // Cargar vertices
    const auto& verticesArray = meshData["Vertices"];
    for (const auto& v : verticesArray)
    {
        MeshVertex vertex;

        if (v.contains("Position"))
        {
            vertex.Position = glm::vec3(
                v["Position"][0].get<float>(),
                v["Position"][1].get<float>(),
                v["Position"][2].get<float>()
            );
        }

        if (v.contains("Normal"))
        {
            vertex.Normal = glm::vec3(
                v["Normal"][0].get<float>(),
                v["Normal"][1].get<float>(),
                v["Normal"][2].get<float>()
            );
        }

        if (v.contains("TexCoords"))
        {
            vertex.TexCoords = glm::vec2(
                v["TexCoords"][0].get<float>(),
                v["TexCoords"][1].get<float>()
            );
        }

        if (v.contains("Tangent"))
        {
            vertex.Tangent = glm::vec3(
                v["Tangent"][0].get<float>(),
                v["Tangent"][1].get<float>(),
                v["Tangent"][2].get<float>()
            );
        }
        else
        {
            vertex.Tangent = glm::vec3(0.0f);
        }

        if (v.contains("Bitangent"))
        {
            vertex.Bitangent = glm::vec3(
                v["Bitangent"][0].get<float>(),
                v["Bitangent"][1].get<float>(),
                v["Bitangent"][2].get<float>()
            );
        }
        else
        {
            vertex.Bitangent = glm::vec3(0.0f);
        }

        vertices.push_back(vertex);
    }

    // Cargar indices
    const auto& indicesArray = meshData["Indices"];
    for (const auto& idx : indicesArray)
    {
        indices.push_back(idx.get<unsigned int>());
    }

    // Cargar AABB
    if (meshData.contains("AABB_Min") && meshData.contains("AABB_Max"))
    {
        localAABB.min = glm::vec3(
            meshData["AABB_Min"][0].get<float>(),
            meshData["AABB_Min"][1].get<float>(),
            meshData["AABB_Min"][2].get<float>()
        );

        localAABB.max = glm::vec3(
            meshData["AABB_Max"][0].get<float>(),
            meshData["AABB_Max"][1].get<float>(),
            meshData["AABB_Max"][2].get<float>()
        );

        aabbDirty = false;
    }

    // Marcar flat vertices como dirty para regenerar
    flatVerticesDirty = true;

    // Actualizar contadores
    numVertices = vertices.size();
    numIndices = indices.size();

    // Recrear buffers de OpenGL
    SetupMesh();
}

void ComponentMesh::CalculateAABB()
{
    if (vertices.empty())
    {
        localAABB.min = glm::vec3(0.0f);
        localAABB.max = glm::vec3(0.0f);
        return;
    }

    glm::vec3 min = vertices[0].Position;
    glm::vec3 max = vertices[0].Position;

    for (const auto& vertex : vertices)
    {
        min = glm::min(min, vertex.Position);
        max = glm::max(max, vertex.Position);
    }

    localAABB.min = min;
    localAABB.max = max;
}