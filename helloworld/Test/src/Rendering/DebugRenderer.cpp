#include "DebugRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <fmt/core.h>

void DebugRenderer::Initialize()
{
    if (isInitialized)
        return;

    SetupBuffers();
    CreateShader();
    isInitialized = true;
}

void DebugRenderer::Cleanup()
{
    if (VAO != 0)
    {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO != 0)
    {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (shaderProgram != 0)
    {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }

    lines.clear();
    isInitialized = false;
}

void DebugRenderer::SetupBuffers()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    // Color attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void DebugRenderer::CreateShader()
{
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;
        
        uniform mat4 uViewProjection;
        
        out vec3 vColor;
        
        void main()
        {
            gl_Position = uViewProjection * vec4(aPos, 1.0);
            vColor = aColor;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        
        void main()
        {
            FragColor = vec4(vColor, 1.0);
        }
    )";

    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        fmt::print("ERROR: Debug vertex shader compilation failed: {}\n", infoLog);
    }

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        fmt::print("ERROR: Debug fragment shader compilation failed: {}\n", infoLog);
    }

    // Link shaders
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        fmt::print("ERROR: Debug shader program linking failed: {}\n", infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void DebugRenderer::DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color)
{
    if (!isEnabled)
        return;

    lines.push_back({ from, to, color });
}

void DebugRenderer::DrawAABB(const AABB& aabb, const glm::vec3& color)
{
    if (!isEnabled || !aabb.IsValid())
        return;

    glm::vec3 corners[8];
    aabb.GetCorners(corners);

    // Bottom face
    DrawLine(corners[0], corners[4], color);
    DrawLine(corners[4], corners[5], color);
    DrawLine(corners[5], corners[1], color);
    DrawLine(corners[1], corners[0], color);

    // Top face
    DrawLine(corners[2], corners[6], color);
    DrawLine(corners[6], corners[7], color);
    DrawLine(corners[7], corners[3], color);
    DrawLine(corners[3], corners[2], color);

    // Vertical edges
    DrawLine(corners[0], corners[2], color);
    DrawLine(corners[1], corners[3], color);
    DrawLine(corners[4], corners[6], color);
    DrawLine(corners[5], corners[7], color);
}

void DebugRenderer::DrawFrustum(const Frustum& frustum, const glm::vec3& color)
{
    if (!isEnabled)
        return;

    // This is a simplified frustum visualization
    // You could extract the 8 corners from the planes and draw them
    // For now, we'll draw the frustum planes as lines from origin
    // A more complete implementation would extract corner points from the planes
    
    // Note: This is a placeholder - you'd need to compute the actual frustum corners
    // from the planes to draw a proper frustum wireframe
}

void DebugRenderer::DrawRay(const glm::vec3& origin, const glm::vec3& direction, float length, const glm::vec3& color)
{
    if (!isEnabled)
        return;

    glm::vec3 end = origin + glm::normalize(direction) * length;
    DrawLine(origin, end, color);
}

void DebugRenderer::DrawSphere(const glm::vec3& center, float radius, const glm::vec3& color, int segments)
{
    if (!isEnabled)
        return;

    float angleStep = glm::two_pi<float>() / segments;

    // Draw circles on XY, XZ, and YZ planes
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;

        // XY plane
        glm::vec3 p1 = center + glm::vec3(cos(angle1) * radius, sin(angle1) * radius, 0.0f);
        glm::vec3 p2 = center + glm::vec3(cos(angle2) * radius, sin(angle2) * radius, 0.0f);
        DrawLine(p1, p2, color);

        // XZ plane
        p1 = center + glm::vec3(cos(angle1) * radius, 0.0f, sin(angle1) * radius);
        p2 = center + glm::vec3(cos(angle2) * radius, 0.0f, sin(angle2) * radius);
        DrawLine(p1, p2, color);

        // YZ plane
        p1 = center + glm::vec3(0.0f, cos(angle1) * radius, sin(angle1) * radius);
        p2 = center + glm::vec3(0.0f, cos(angle2) * radius, sin(angle2) * radius);
        DrawLine(p1, p2, color);
    }
}

void DebugRenderer::Render(const glm::mat4& viewProjection)
{
    if (!isEnabled || !isInitialized || lines.empty())
        return;

    // Prepare vertex data
    std::vector<float> vertices;
    vertices.reserve(lines.size() * 12); // 2 vertices * 6 floats per line

    for (const auto& line : lines)
    {
        // Start vertex
        vertices.push_back(line.start.x);
        vertices.push_back(line.start.y);
        vertices.push_back(line.start.z);
        vertices.push_back(line.color.r);
        vertices.push_back(line.color.g);
        vertices.push_back(line.color.b);

        // End vertex
        vertices.push_back(line.end.x);
        vertices.push_back(line.end.y);
        vertices.push_back(line.end.z);
        vertices.push_back(line.color.r);
        vertices.push_back(line.color.g);
        vertices.push_back(line.color.b);
    }

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    // Render
    glUseProgram(shaderProgram);
    GLint vpLoc = glGetUniformLocation(shaderProgram, "uViewProjection");
    glUniformMatrix4fv(vpLoc, 1, GL_FALSE, glm::value_ptr(viewProjection));

    glBindVertexArray(VAO);

    // Disable depth test for debug lines so they're always visible
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);

    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size() * 2));
    glLineWidth(1.0f);

    if (depthTestEnabled)
        glEnable(GL_DEPTH_TEST);

    glBindVertexArray(0);
    glUseProgram(0);
}

void DebugRenderer::Clear()
{
    lines.clear();
}
