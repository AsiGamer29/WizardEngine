#pragma once
#include "BaseComponent.h"
#include <string>
#include <glad/glad.h>

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

private:
    void CleanUp();
    void SetupBlendMode();
};