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

    // Generar vértices
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
            vertex.Normal = glm::normalize(glm::vec3(x, y, z)); // Normal apunta HACIA FUERA
            vertex.TexCoords = glm::vec2(float(seg) / segments, float(ring) / rings);

            mesh.vertices.push_back(vertex);
        }
    }

    // Generar índices - ORDEN CORRECTO (counter-clockwise)
    for (int ring = 0; ring < rings; ++ring)
    {
        for (int seg = 0; seg < segments; ++seg)
        {
            int current = ring * (segments + 1) + seg;
            int next = current + segments + 1;

            // Primer triángulo (counter-clockwise desde fuera)
            mesh.indices.push_back(current);
            mesh.indices.push_back(current + 1);
            mesh.indices.push_back(next);

            // Segundo triángulo (counter-clockwise desde fuera)
            mesh.indices.push_back(current + 1);
            mesh.indices.push_back(next + 1);
            mesh.indices.push_back(next);
        }
    }

    return mesh;
}

MeshGeometry GeometryGenerator::CreateCylinder(float radius, float height, int segments)
{
    MeshGeometry mesh;
    float halfHeight = height / 2.0f;

    // === LATERAL DEL CILINDRO ===
    for (int i = 0; i <= segments; ++i)
    {
        float theta = 2.0f * M_PI * float(i) / float(segments);
        float x = radius * cos(theta);
        float z = radius * sin(theta);

        glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));

        // Par de vértices: superior e inferior
        GeomVertex topVertex;
        topVertex.Position = glm::vec3(x, halfHeight, z);
        topVertex.Normal = normal;
        topVertex.TexCoords = glm::vec2(float(i) / float(segments), 1.0f);
        mesh.vertices.push_back(topVertex);

        GeomVertex bottomVertex;
        bottomVertex.Position = glm::vec3(x, -halfHeight, z);
        bottomVertex.Normal = normal;
        bottomVertex.TexCoords = glm::vec2(float(i) / float(segments), 0.0f);
        mesh.vertices.push_back(bottomVertex);
    }

    // Índices del lateral
    for (int i = 0; i < segments; ++i)
    {
        int current = i * 2;
        int next = current + 2;

        // Triángulo 1
        mesh.indices.push_back(current + 1);  // bottom current
        mesh.indices.push_back(current);      // top current
        mesh.indices.push_back(next);         // top next

        // Triángulo 2
        mesh.indices.push_back(current + 1);  // bottom current
        mesh.indices.push_back(next);         // top next
        mesh.indices.push_back(next + 1);     // bottom next
    }

    // === TAPA SUPERIOR ===
    int topCapStart = mesh.vertices.size();

    // Centro
    GeomVertex topCenter;
    topCenter.Position = glm::vec3(0.0f, halfHeight, 0.0f);
    topCenter.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
    topCenter.TexCoords = glm::vec2(0.5f, 0.5f);
    mesh.vertices.push_back(topCenter);

    // Anillo superior
    for (int i = 0; i <= segments; ++i)
    {
        float theta = 2.0f * M_PI * float(i) / float(segments);
        float x = radius * cos(theta);
        float z = radius * sin(theta);

        GeomVertex v;
        v.Position = glm::vec3(x, halfHeight, z);
        v.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        v.TexCoords = glm::vec2(0.5f + 0.5f * x / radius, 0.5f + 0.5f * z / radius);
        mesh.vertices.push_back(v);
    }

    for (int i = 0; i < segments; ++i)
    {
        mesh.indices.push_back(topCapStart);           // centro
        mesh.indices.push_back(topCapStart + 1 + i + 1); // siguiente
        mesh.indices.push_back(topCapStart + 1 + i);     // actual
    }

    // === TAPA INFERIOR ===
    int bottomCapStart = mesh.vertices.size();

    // Centro
    GeomVertex bottomCenter;
    bottomCenter.Position = glm::vec3(0.0f, -halfHeight, 0.0f);
    bottomCenter.Normal = glm::vec3(0.0f, -1.0f, 0.0f);
    bottomCenter.TexCoords = glm::vec2(0.5f, 0.5f);
    mesh.vertices.push_back(bottomCenter);

    // Anillo inferior
    for (int i = 0; i <= segments; ++i)
    {
        float theta = 2.0f * M_PI * float(i) / float(segments);
        float x = radius * cos(theta);
        float z = radius * sin(theta);

        GeomVertex v;
        v.Position = glm::vec3(x, -halfHeight, z);
        v.Normal = glm::vec3(0.0f, -1.0f, 0.0f);
        v.TexCoords = glm::vec2(0.5f + 0.5f * x / radius, 0.5f - 0.5f * z / radius);
        mesh.vertices.push_back(v);
    }

    for (int i = 0; i < segments; ++i)
    {
        mesh.indices.push_back(bottomCapStart);           // centro
        mesh.indices.push_back(bottomCapStart + 1 + i);     // actual
        mesh.indices.push_back(bottomCapStart + 1 + i + 1); // siguiente
    }

    return mesh;
}

MeshGeometry GeometryGenerator::CreatePyramid(float base, float height)
{
    MeshGeometry mesh;
    float halfBase = base / 2.0f;

    // Vértices de la base
    glm::vec3 v0(-halfBase, 0.0f, halfBase);
    glm::vec3 v1(halfBase, 0.0f, halfBase);
    glm::vec3 v2(halfBase, 0.0f, -halfBase);
    glm::vec3 v3(-halfBase, 0.0f, -halfBase);
    glm::vec3 apex(0.0f, height, 0.0f);

    // BASE (normal hacia abajo)
    mesh.vertices.push_back({ v0, {0, -1, 0}, {0, 0} });
    mesh.vertices.push_back({ v1, {0, -1, 0}, {1, 0} });
    mesh.vertices.push_back({ v2, {0, -1, 0}, {1, 1} });
    mesh.vertices.push_back({ v3, {0, -1, 0}, {0, 1} });

    mesh.indices.push_back(0); mesh.indices.push_back(2); mesh.indices.push_back(1);
    mesh.indices.push_back(0); mesh.indices.push_back(3); mesh.indices.push_back(2);

    // CARA FRONTAL (v0, v1, apex)
    glm::vec3 nFront = glm::normalize(glm::cross(v1 - v0, apex - v0));
    int baseFront = mesh.vertices.size();
    mesh.vertices.push_back({ v0, nFront, {0, 0} });
    mesh.vertices.push_back({ v1, nFront, {1, 0} });
    mesh.vertices.push_back({ apex, nFront, {0.5f, 1} });
    mesh.indices.push_back(baseFront);
    mesh.indices.push_back(baseFront + 1);
    mesh.indices.push_back(baseFront + 2);

    // CARA DERECHA (v1, v2, apex)
    glm::vec3 nRight = glm::normalize(glm::cross(v2 - v1, apex - v1));
    int baseRight = mesh.vertices.size();
    mesh.vertices.push_back({ v1, nRight, {0, 0} });
    mesh.vertices.push_back({ v2, nRight, {1, 0} });
    mesh.vertices.push_back({ apex, nRight, {0.5f, 1} });
    mesh.indices.push_back(baseRight);
    mesh.indices.push_back(baseRight + 1);
    mesh.indices.push_back(baseRight + 2);

    // CARA TRASERA (v2, v3, apex)
    glm::vec3 nBack = glm::normalize(glm::cross(v3 - v2, apex - v2));
    int baseBack = mesh.vertices.size();
    mesh.vertices.push_back({ v2, nBack, {0, 0} });
    mesh.vertices.push_back({ v3, nBack, {1, 0} });
    mesh.vertices.push_back({ apex, nBack, {0.5f, 1} });
    mesh.indices.push_back(baseBack);
    mesh.indices.push_back(baseBack + 1);
    mesh.indices.push_back(baseBack + 2);

    // CARA IZQUIERDA (v3, v0, apex)
    glm::vec3 nLeft = glm::normalize(glm::cross(v0 - v3, apex - v3));
    int baseLeft = mesh.vertices.size();
    mesh.vertices.push_back({ v3, nLeft, {0, 0} });
    mesh.vertices.push_back({ v0, nLeft, {1, 0} });
    mesh.vertices.push_back({ apex, nLeft, {0.5f, 1} });
    mesh.indices.push_back(baseLeft);
    mesh.indices.push_back(baseLeft + 1);
    mesh.indices.push_back(baseLeft + 2);

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