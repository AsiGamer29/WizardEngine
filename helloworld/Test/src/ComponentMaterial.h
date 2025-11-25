#pragma once
#include "BaseComponent.h"
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

enum class AlphaMode
{
    OPAQUE,
    ALPHA_TEST,
    ALPHA_BLEND
};

enum class BlendMode
{
    STANDARD,
    ADDITIVE,
    MULTIPLY,
    SCREEN,
    PREMULTIPLIED
};

enum class ShaderType
{
    UNLIT,
    VERTEX_PHONG,
    PIXEL_PHONG,
    PIXEL_BLINN_PHONG
};

class ComponentMaterial : public Component
{
private:
    GLuint textureID;
    std::string texturePath;

    int width;
    int height;
    int channels;

    GLuint overrideTextureID = 0;
    bool overrideTextureOwned = false;

    AlphaMode alphaMode = AlphaMode::OPAQUE;
    float alphaCutoff = 0.5f;
    BlendMode blendMode = BlendMode::STANDARD;

    // SHADER AND LIGHTING PROPERTIES
    ShaderType shaderType = ShaderType::PIXEL_PHONG;
    glm::vec3 ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
    glm::vec3 diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float shininess = 32.0f;
    glm::vec4 colorTint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

public:
    ComponentMaterial(GameObject* owner);
    ~ComponentMaterial();

    void LoadTexture(const char* path);
    void SetTexture(GLuint texID, const char* path = "", int texChannels = 0);

    void SetOverrideTexture(GLuint texID, bool takeOwnership = false);
    void ClearOverrideTexture();
    GLuint GetOverrideTextureID() const { return overrideTextureID; }

    void Bind();
    void Unbind();
    void OnEditor() override;

    GLuint GetTextureID() const { return textureID; }
    const char* GetTexturePath() const { return texturePath.c_str(); }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

    AlphaMode GetAlphaMode() const { return alphaMode; }
    void SetAlphaMode(AlphaMode mode) { alphaMode = mode; }

    float GetAlphaCutoff() const { return alphaCutoff; }
    void SetAlphaCutoff(float cutoff) { alphaCutoff = cutoff; }

    BlendMode GetBlendMode() const { return blendMode; }
    void SetBlendMode(BlendMode mode) { blendMode = mode; }

    bool NeedsBlending() const { return alphaMode == AlphaMode::ALPHA_BLEND; }
    bool IsOpaque() const { return alphaMode == AlphaMode::OPAQUE; }

    // SHADER AND LIGHTING GETTERS/SETTERS
    ShaderType GetShaderType() const { return shaderType; }
    void SetShaderType(ShaderType type) { shaderType = type; }

    glm::vec3 GetAmbientColor() const { return ambientColor; }
    void SetAmbientColor(const glm::vec3& color) { ambientColor = color; }

    glm::vec3 GetDiffuseColor() const { return diffuseColor; }
    void SetDiffuseColor(const glm::vec3& color) { diffuseColor = color; }

    glm::vec3 GetSpecularColor() const { return specularColor; }
    void SetSpecularColor(const glm::vec3& color) { specularColor = color; }

    float GetShininess() const { return shininess; }
    void SetShininess(float value) { shininess = value; }

    glm::vec4 GetColorTint() const { return colorTint; }
    void SetColorTint(const glm::vec4& color) { colorTint = color; }

private:
    void CleanUp();
    void SetupBlendMode();
};