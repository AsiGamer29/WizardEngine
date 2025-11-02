#include "GeometryGenerator.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

MeshGeometry GeometryGenerator::CreateCube(float size)
{
    MeshGeometry mesh;
    float halfSize = size / 2.0f;

    // 24 vértices (4 por cara, 6 caras)
    GeomVertex vertices[24] = {
        // Front face (Z+)
        {{ -halfSize, -halfSize,  halfSize }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }},
        {{  halfSize, -halfSize,  halfSize }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }},
        {{  halfSize,  halfSize,  halfSize }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }},
        {{ -halfSize,  halfSize,  halfSize }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }},

        // Back face (Z-)
        {{  halfSize, -halfSize, -halfSize }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f }},
        {{ -halfSize, -halfSize, -halfSize }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f }},
        {{ -halfSize,  halfSize, -halfSize }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f }},
        {{  halfSize,  halfSize, -halfSize }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f }},

        // Top face (Y+)
        {{ -halfSize,  halfSize,  halfSize }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }},
        {{  halfSize,  halfSize,  halfSize }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }},
        {{  halfSize,  halfSize, -halfSize }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }},
        {{ -halfSize,  halfSize, -halfSize }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }},

        // Bottom face (Y-)
        {{ -halfSize, -halfSize, -halfSize }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }},
        {{  halfSize, -halfSize, -halfSize }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }},
        {{  halfSize, -halfSize,  halfSize }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f }},
        {{ -halfSize, -halfSize,  halfSize }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f }},

        // Right face (X+)
        {{  halfSize, -halfSize,  halfSize }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},
        {{  halfSize, -halfSize, -halfSize }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
        {{  halfSize,  halfSize, -halfSize }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }},
        {{  halfSize,  halfSize,  halfSize }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},

        // Left face (X-)
        {{ -halfSize, -halfSize, -halfSize }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},
        {{ -halfSize, -halfSize,  halfSize }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
        {{ -halfSize,  halfSize,  halfSize }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }},
        {{ -halfSize,  halfSize, -halfSize }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
    };

    // Índices (2 triángulos por cara)
    unsigned int indices[36] = {
        0,1,2,    0,2,3,    // Front
        4,5,6,    4,6,7,    // Back
        8,9,10,   8,10,11,  // Top
        12,13,14, 12,14,15, // Bottom
        16,17,18, 16,18,19, // Right
        20,21,22, 20,22,23  // Left
    };

    mesh.vertices.assign(vertices, vertices + 24);
    mesh.indices.assign(indices, indices + 36);

    return mesh;
}

MeshGeometry GeometryGenerator::CreateSphere(float radius, int segments, int rings)
{
    MeshGeometry mesh;

    for (int ring = 0; ring <= rings; ++ring)
    {
        float phi = M_PI * float(ring) / float(rings);
        float y = radius * cos(phi);
        float ringRadius = radius * sin(phi);

        for (int seg = 0; seg <= segments; ++seg)
        {
            float theta = 2.0f * M_PI * float(seg) / float(segments);
            float x = ringRadius * cos(theta);
            float z = ringRadius * sin(theta);

            GeomVertex vertex;
            vertex.Position = glm::vec3(x, y, z);
            vertex.Normal = glm::normalize(glm::vec3(x, y, z));
            vertex.TexCoords = glm::vec2(float(seg) / segments, float(ring) / rings);

            mesh.vertices.push_back(vertex);
        }
    }

    for (int ring = 0; ring < rings; ++ring)
    {
        for (int seg = 0; seg < segments; ++seg)
        {
            int current = ring * (segments + 1) + seg;
            int next = current + segments + 1;

            mesh.indices.push_back(current);
            mesh.indices.push_back(next);
            mesh.indices.push_back(current + 1);

            mesh.indices.push_back(current + 1);
            mesh.indices.push_back(next);
            mesh.indices.push_back(next + 1);
        }
    }

    return mesh;
}

MeshGeometry GeometryGenerator::CreateCylinder(float radius, float height, int segments)
{
    MeshGeometry mesh;
    float halfHeight = height / 2.0f;

    // Top center
    GeomVertex topCenter;
    topCenter.Position = glm::vec3(0.0f, halfHeight, 0.0f);
    topCenter.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
    topCenter.TexCoords = glm::vec2(0.5f, 0.5f);
    mesh.vertices.push_back(topCenter);

    // Bottom center
    GeomVertex bottomCenter;
    bottomCenter.Position = glm::vec3(0.0f, -halfHeight, 0.0f);
    bottomCenter.Normal = glm::vec3(0.0f, -1.0f, 0.0f);
    bottomCenter.TexCoords = glm::vec2(0.5f, 0.5f);
    mesh.vertices.push_back(bottomCenter);

    // Side vertices
    for (int i = 0; i <= segments; ++i)
    {
        float theta = 2.0f * M_PI * float(i) / float(segments);
        float x = radius * cos(theta);
        float z = radius * sin(theta);

        // Top ring
        GeomVertex topVertex;
        topVertex.Position = glm::vec3(x, halfHeight, z);
        topVertex.Normal = glm::normalize(glm::vec3(x, 0.0f, z));
        topVertex.TexCoords = glm::vec2(float(i) / segments, 1.0f);
        mesh.vertices.push_back(topVertex);

        // Bottom ring
        GeomVertex bottomVertex;
        bottomVertex.Position = glm::vec3(x, -halfHeight, z);
        bottomVertex.Normal = glm::normalize(glm::vec3(x, 0.0f, z));
        bottomVertex.TexCoords = glm::vec2(float(i) / segments, 0.0f);
        mesh.vertices.push_back(bottomVertex);
    }

    // Indices for sides
    for (int i = 0; i < segments; ++i)
    {
        int topCurrent = 2 + i * 2;
        int topNext = topCurrent + 2;
        int bottomCurrent = topCurrent + 1;
        int bottomNext = bottomCurrent + 2;

        mesh.indices.push_back(topCurrent);
        mesh.indices.push_back(bottomCurrent);
        mesh.indices.push_back(topNext);

        mesh.indices.push_back(topNext);
        mesh.indices.push_back(bottomCurrent);
        mesh.indices.push_back(bottomNext);

        // Top cap
        mesh.indices.push_back(0);
        mesh.indices.push_back(topNext);
        mesh.indices.push_back(topCurrent);

        // Bottom cap
        mesh.indices.push_back(1);
        mesh.indices.push_back(bottomCurrent);
        mesh.indices.push_back(bottomNext);
    }

    return mesh;
}

MeshGeometry GeometryGenerator::CreatePyramid(float base, float height)
{
    MeshGeometry mesh;
    float halfBase = base / 2.0f;

    GeomVertex vertices[5] = {
        // Base
        {{ -halfBase, 0.0f,  halfBase }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }},
        {{  halfBase, 0.0f,  halfBase }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }},
        {{  halfBase, 0.0f, -halfBase }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f }},
        {{ -halfBase, 0.0f, -halfBase }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f }},
        // Apex
        {{ 0.0f, height, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.5f }},
    };

    unsigned int indices[18] = {
        0,2,1,  0,3,2,  // Base
        0,1,4,          // Front
        1,2,4,          // Right
        2,3,4,          // Back
        3,0,4           // Left
    };

    mesh.vertices.assign(vertices, vertices + 5);
    mesh.indices.assign(indices, indices + 18);

    return mesh;
}

MeshGeometry GeometryGenerator::CreatePlane(float width, float depth)
{
    MeshGeometry mesh;
    float halfW = width / 2.0f;
    float halfD = depth / 2.0f;

    GeomVertex vertices[4] = {
        {{ -halfW, 0.0f,  halfD }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }},
        {{  halfW, 0.0f,  halfD }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }},
        {{  halfW, 0.0f, -halfD }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }},
        {{ -halfW, 0.0f, -halfD }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }},
    };

    unsigned int indices[6] = { 0, 1, 2, 0, 2, 3 };

    mesh.vertices.assign(vertices, vertices + 4);
    mesh.indices.assign(indices, indices + 6);

    return mesh;
}

void MeshGeometry::SetupMesh()
{
    // Ya implementado en tu código existente
}

void MeshGeometry::Draw()
{
    // Ya implementado en tu código existente
}

void MeshGeometry::Cleanup()
{
    // Ya implementado en tu código existente
}