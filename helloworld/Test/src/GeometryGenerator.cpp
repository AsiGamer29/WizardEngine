#include "GeometryGenerator.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void MeshGeometry::SetupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GeomVertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GeomVertex), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GeomVertex), (void*)offsetof(GeomVertex, Normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GeomVertex), (void*)offsetof(GeomVertex, TexCoords));

    glBindVertexArray(0);
}

void MeshGeometry::Draw() {
    if (VAO == 0) return;
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void MeshGeometry::Cleanup() {
    if (EBO) glDeleteBuffers(1, &EBO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (VAO) glDeleteVertexArrays(1, &VAO);
    VAO = VBO = EBO = 0;
}

MeshGeometry GeometryGenerator::CreateCube(float size) {
    MeshGeometry mesh;
    float h = size / 2.0f;

    mesh.vertices = {
        {{-h, -h,  h}, {0, 0, 1}, {0, 0}}, {{ h, -h,  h}, {0, 0, 1}, {1, 0}},
        {{ h,  h,  h}, {0, 0, 1}, {1, 1}}, {{-h,  h,  h}, {0, 0, 1}, {0, 1}},
        {{ h, -h, -h}, {0, 0, -1}, {0, 0}}, {{-h, -h, -h}, {0, 0, -1}, {1, 0}},
        {{-h,  h, -h}, {0, 0, -1}, {1, 1}}, {{ h,  h, -h}, {0, 0, -1}, {0, 1}},
        {{-h,  h,  h}, {0, 1, 0}, {0, 0}}, {{ h,  h,  h}, {0, 1, 0}, {1, 0}},
        {{ h,  h, -h}, {0, 1, 0}, {1, 1}}, {{-h,  h, -h}, {0, 1, 0}, {0, 1}},
        {{-h, -h, -h}, {0, -1, 0}, {0, 0}}, {{ h, -h, -h}, {0, -1, 0}, {1, 0}},
        {{ h, -h,  h}, {0, -1, 0}, {1, 1}}, {{-h, -h,  h}, {0, -1, 0}, {0, 1}},
        {{ h, -h,  h}, {1, 0, 0}, {0, 0}}, {{ h, -h, -h}, {1, 0, 0}, {1, 0}},
        {{ h,  h, -h}, {1, 0, 0}, {1, 1}}, {{ h,  h,  h}, {1, 0, 0}, {0, 1}},
        {{-h, -h, -h}, {-1, 0, 0}, {0, 0}}, {{-h, -h,  h}, {-1, 0, 0}, {1, 0}},
        {{-h,  h,  h}, {-1, 0, 0}, {1, 1}}, {{-h,  h, -h}, {-1, 0, 0}, {0, 1}}
    };

    mesh.indices = {
        0,1,2, 0,2,3,   4,5,6, 4,6,7,   8,9,10, 8,10,11,
        12,13,14, 12,14,15,   16,17,18, 16,18,19,   20,21,22, 20,22,23
    };

    mesh.SetupMesh();
    return mesh;
}

MeshGeometry GeometryGenerator::CreateSphere(float radius, int segments, int rings) {
    MeshGeometry mesh;

    // Vértices
    for (int r = 0; r <= rings; ++r) {
        float phi = M_PI * (float)r / (float)rings;
        float y = cos(phi);
        float ringRadius = sin(phi);

        for (int s = 0; s <= segments; ++s) {
            float theta = 2.0f * M_PI * (float)s / (float)segments;
            float x = ringRadius * cos(theta);
            float z = ringRadius * sin(theta);

            glm::vec3 pos(x * radius, y * radius, z * radius);
            glm::vec3 normal(x, y, z);
            glm::vec2 uv((float)s / (float)segments, (float)r / (float)rings);

            mesh.vertices.push_back({ pos, normal, uv });
        }
    }

    // Índices - CUIDADOSAMENTE verificados
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < segments; ++s) {
            int curr = r * (segments + 1) + s;
            int next = curr + segments + 1;

            // Triángulo 1
            mesh.indices.push_back(curr);
            mesh.indices.push_back(curr + 1);
            mesh.indices.push_back(next);

            // Triángulo 2
            mesh.indices.push_back(curr + 1);
            mesh.indices.push_back(next + 1);
            mesh.indices.push_back(next);
        }
    }

    mesh.SetupMesh();
    return mesh;
}

MeshGeometry GeometryGenerator::CreateCylinder(float radius, float height, int segments) {
    MeshGeometry mesh;
    float halfH = height / 2.0f;

    // ===== LATERAL =====
    for (int s = 0; s <= segments; ++s) {
        float theta = 2.0f * M_PI * s / segments;
        float x = cos(theta) * radius;
        float z = sin(theta) * radius;

        glm::vec3 normal = glm::normalize(glm::vec3(x, 0, z));

        // Vértice superior e inferior (lado frontal)
        mesh.vertices.push_back({ {x, halfH, z}, normal, { (float)s / segments, 1.0f } });
        mesh.vertices.push_back({ {x, -halfH, z}, normal, { (float)s / segments, 0.0f } });

        // Vértice superior e inferior (lado trasero, normales invertidas)
        mesh.vertices.push_back({ {x, halfH, z}, -normal, { (float)s / segments, 1.0f } });
        mesh.vertices.push_back({ {x, -halfH, z}, -normal, { (float)s / segments, 0.0f } });
    }

    // Índices laterales (frontal)
    for (int s = 0; s < segments; ++s) {
        int top1 = s * 4;
        int bottom1 = s * 4 + 1;
        int top2 = (s + 1) * 4;
        int bottom2 = (s + 1) * 4 + 1;

        // Triángulo 1
        mesh.indices.push_back(top1);
        mesh.indices.push_back(bottom1);
        mesh.indices.push_back(top2);

        // Triángulo 2
        mesh.indices.push_back(top2);
        mesh.indices.push_back(bottom1);
        mesh.indices.push_back(bottom2);

        // Triángulo 1 (trasero)
        top1 += 2; bottom1 += 2; top2 += 2; bottom2 += 2;
        mesh.indices.push_back(top1);
        mesh.indices.push_back(top2);
        mesh.indices.push_back(bottom1);

        // Triángulo 2 (trasero)
        mesh.indices.push_back(top2);
        mesh.indices.push_back(bottom2);
        mesh.indices.push_back(bottom1);
    }

    // ===== TAPA SUPERIOR =====
    int topCenter = mesh.vertices.size();
    mesh.vertices.push_back({ {0, halfH, 0}, {0, 1, 0}, {0.5f, 0.5f} });       // frontal
    mesh.vertices.push_back({ {0, halfH, 0}, {0, -1, 0}, {0.5f, 0.5f} });      // trasera

    for (int s = 0; s <= segments; ++s) {
        float theta = 2.0f * M_PI * s / segments;
        float x = cos(theta) * radius;
        float z = sin(theta) * radius;

        // Anillo superior frontal
        mesh.vertices.push_back({ {x, halfH, z}, {0, 1, 0}, {0.5f + 0.5f * x / radius, 0.5f + 0.5f * z / radius} });
        // Anillo superior trasero
        mesh.vertices.push_back({ {x, halfH, z}, {0, -1, 0}, {0.5f + 0.5f * x / radius, 0.5f + 0.5f * z / radius} });
    }

    for (int s = 0; s < segments; ++s) {
        int centerFront = topCenter;
        int centerBack = topCenter + 1;
        int ringFront1 = topCenter + 2 + s * 2;
        int ringFront2 = topCenter + 2 + (s + 1) * 2;
        int ringBack1 = topCenter + 3 + s * 2;
        int ringBack2 = topCenter + 3 + (s + 1) * 2;

        // frontal
        mesh.indices.push_back(centerFront);
        mesh.indices.push_back(ringFront1);
        mesh.indices.push_back(ringFront2);

        // trasera (winding invertido)
        mesh.indices.push_back(centerBack);
        mesh.indices.push_back(ringBack2);
        mesh.indices.push_back(ringBack1);
    }

    // ===== TAPA INFERIOR =====
    int bottomCenter = mesh.vertices.size();
    mesh.vertices.push_back({ {0, -halfH, 0}, {0, -1, 0}, {0.5f, 0.5f} });     // frontal
    mesh.vertices.push_back({ {0, -halfH, 0}, {0, 1, 0}, {0.5f, 0.5f} });      // trasera

    for (int s = 0; s <= segments; ++s) {
        float theta = 2.0f * M_PI * s / segments;
        float x = cos(theta) * radius;
        float z = sin(theta) * radius;

        // Anillo inferior frontal
        mesh.vertices.push_back({ {x, -halfH, z}, {0, -1, 0}, {0.5f + 0.5f * x / radius, 0.5f + 0.5f * z / radius} });
        // Anillo inferior trasero
        mesh.vertices.push_back({ {x, -halfH, z}, {0, 1, 0}, {0.5f + 0.5f * x / radius, 0.5f + 0.5f * z / radius} });
    }

    for (int s = 0; s < segments; ++s) {
        int centerFront = bottomCenter;
        int centerBack = bottomCenter + 1;
        int ringFront1 = bottomCenter + 2 + s * 2;
        int ringFront2 = bottomCenter + 2 + (s + 1) * 2;
        int ringBack1 = bottomCenter + 3 + s * 2;
        int ringBack2 = bottomCenter + 3 + (s + 1) * 2;

        // frontal
        mesh.indices.push_back(centerFront);
        mesh.indices.push_back(ringFront2);
        mesh.indices.push_back(ringFront1);

        // trasera (winding invertido)
        mesh.indices.push_back(centerBack);
        mesh.indices.push_back(ringBack1);
        mesh.indices.push_back(ringBack2);
    }

    mesh.SetupMesh();
    return mesh;
}

MeshGeometry GeometryGenerator::CreatePyramid(float base, float height) {
    MeshGeometry mesh;
    float h = base / 2.0f;

    // Posiciones
    glm::vec3 p0(-h, 0, -h);
    glm::vec3 p1(h, 0, -h);
    glm::vec3 p2(h, 0, h);
    glm::vec3 p3(-h, 0, h);
    glm::vec3 apex(0, height, 0);

    // BASE (2 triángulos)
    glm::vec3 nBase(0, -1, 0);
    mesh.vertices.push_back({ p0, nBase, {0, 0} });  // 0
    mesh.vertices.push_back({ p1, nBase, {1, 0} });  // 1
    mesh.vertices.push_back({ p2, nBase, {1, 1} });  // 2
    mesh.vertices.push_back({ p3, nBase, {0, 1} });  // 3
    mesh.indices.push_back(0);
    mesh.indices.push_back(2);
    mesh.indices.push_back(1);
    mesh.indices.push_back(0);
    mesh.indices.push_back(3);
    mesh.indices.push_back(2);

    // CARA FRONTAL (p0 -> p1 -> apex)
    glm::vec3 e1 = p1 - p0;
    glm::vec3 e2 = apex - p0;
    glm::vec3 nFront = glm::normalize(glm::cross(e1, e2));
    int frontBase = mesh.vertices.size();
    mesh.vertices.push_back({ p0, nFront, {0, 0} });
    mesh.vertices.push_back({ p1, nFront, {1, 0} });
    mesh.vertices.push_back({ apex, nFront, {0.5f, 1} });
    mesh.indices.push_back(frontBase);
    mesh.indices.push_back(frontBase + 2);
    mesh.indices.push_back(frontBase + 1);

    // CARA DERECHA (p1 -> p2 -> apex)
    e1 = p2 - p1;
    e2 = apex - p1;
    glm::vec3 nRight = glm::normalize(glm::cross(e1, e2));
    int rightBase = mesh.vertices.size();
    mesh.vertices.push_back({ p1, nRight, {0, 0} });
    mesh.vertices.push_back({ p2, nRight, {1, 0} });
    mesh.vertices.push_back({ apex, nRight, {0.5f, 1} });
    mesh.indices.push_back(rightBase);
    mesh.indices.push_back(rightBase + 2);
    mesh.indices.push_back(rightBase + 1);

    // CARA TRASERA (p2 -> p3 -> apex)
    e1 = p3 - p2;
    e2 = apex - p2;
    glm::vec3 nBack = glm::normalize(glm::cross(e1, e2));
    int backBase = mesh.vertices.size();
    mesh.vertices.push_back({ p2, nBack, {0, 0} });
    mesh.vertices.push_back({ p3, nBack, {1, 0} });
    mesh.vertices.push_back({ apex, nBack, {0.5f, 1} });
    mesh.indices.push_back(backBase);
    mesh.indices.push_back(backBase + 2);
    mesh.indices.push_back(backBase + 1);

    // CARA IZQUIERDA (p3 -> p0 -> apex)
    e1 = p0 - p3;
    e2 = apex - p3;
    glm::vec3 nLeft = glm::normalize(glm::cross(e1, e2));
    int leftBase = mesh.vertices.size();
    mesh.vertices.push_back({ p3, nLeft, {0, 0} });
    mesh.vertices.push_back({ p0, nLeft, {1, 0} });
    mesh.vertices.push_back({ apex, nLeft, {0.5f, 1} });
    mesh.indices.push_back(leftBase);
    mesh.indices.push_back(leftBase + 2);
    mesh.indices.push_back(leftBase + 1);

    mesh.SetupMesh();
    return mesh;
}

MeshGeometry GeometryGenerator::CreatePlane(float width, float depth) {
    MeshGeometry mesh;
    float hw = width / 2.0f;
    float hd = depth / 2.0f;

    mesh.vertices = {
        {{-hw, 0, -hd}, {0, 1, 0}, {0, 0}},
        {{ hw, 0, -hd}, {0, 1, 0}, {1, 0}},
        {{ hw, 0,  hd}, {0, 1, 0}, {1, 1}},
        {{-hw, 0,  hd}, {0, 1, 0}, {0, 1}}
    };

    mesh.indices = { 0, 2, 1, 0, 3, 2 };

    mesh.SetupMesh();
    return mesh;
}