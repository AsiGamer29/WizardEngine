#pragma once
#include "BaseComponent.h"
#include <string>
#include <glad/glad.h>

class ComponentMaterial : public Component
{
private:
    GLuint textureID;
    std::string texturePath;

    int width;
    int height;
    int channels;

    // Optional override texture used only for rendering (not replacing original texture)
    GLuint overrideTextureID = 0;
    bool overrideTextureOwned = false;

public:
    ComponentMaterial(GameObject* owner);
    ~ComponentMaterial();

    // Carga desde archivo (usando DevIL como en tu Texture.cpp)
    void LoadTexture(const char* path);

    // Asigna una textura ya cargada (para usar con tu sistema actual)
    void SetTexture(GLuint texID, const char* path = "");

    // Override: asigna una textura temporal para usar en Bind() sin perder la referencia original
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

private:
    void CleanUp();
};