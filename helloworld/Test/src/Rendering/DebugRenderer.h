#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <glad/glad.h>
#include "AABB.h"
#include "Frustum.h"

class DebugRenderer
{
public:
    static DebugRenderer& Get()
    {
        static DebugRenderer instance;
        return instance;
    }

    void Initialize();
    void Cleanup();

    // Draw functions
    void DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f));
    void DrawAABB(const AABB& aabb, const glm::vec3& color = glm::vec3(0.0f, 1.0f, 0.0f));
    void DrawFrustum(const Frustum& frustum, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 0.0f));
    void DrawRay(const glm::vec3& origin, const glm::vec3& direction, float length = 1000.0f, const glm::vec3& color = glm::vec3(1.0f, 0.0f, 0.0f));
    void DrawSphere(const glm::vec3& center, float radius, const glm::vec3& color = glm::vec3(1.0f, 0.0f, 1.0f), int segments = 16);

    // Render all debug geometry
    void Render(const glm::mat4& viewProjection);

    // Clear all debug geometry
    void Clear();

    // Enable/Disable debug rendering
    void SetEnabled(bool enabled) { isEnabled = enabled; }
    bool IsEnabled() const { return isEnabled; }

private:
    DebugRenderer() = default;
    ~DebugRenderer() = default;

    DebugRenderer(const DebugRenderer&) = delete;
    DebugRenderer& operator=(const DebugRenderer&) = delete;

    struct DebugLine
    {
        glm::vec3 start;
        glm::vec3 end;
        glm::vec3 color;
    };

    std::vector<DebugLine> lines;

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint shaderProgram = 0;

    bool isEnabled = true;
    bool isInitialized = false;

    void SetupBuffers();
    void CreateShader();
};
