#include "ComponentMaterial.h"
#include "Texture.h"
#include "imgui.h"
#include <iostream>

ComponentMaterial::ComponentMaterial(GameObject* owner)
    : Component(owner, ComponentType::MATERIAL), textureID(0), width(0), height(0), channels(0)
{
}

ComponentMaterial::~ComponentMaterial()
{
    // NO llamar CleanUp() aquí - las texturas se gestionan por el sistema de caché
    // o son compartidas (checkerboard). Liberar aquí causa crashes al restaurar escenas.
    
    // Solo limpiar override si lo poseemos
    if (overrideTextureOwned && overrideTextureID != 0)
    {
        glDeleteTextures(1, &overrideTextureID);
    }
}

void ComponentMaterial::LoadTexture(const char* path)
{
    if (!path || path[0] == '\0')
    {
        std::cerr << "[Material] LoadTexture called with empty path" << std::endl;
        return;
    }

    // Limpiar textura anterior
    if (textureID != 0 && !texturePath.empty() && texturePath != "checkerboard_default")
    {
        Texture::ReleaseTexture(texturePath);
        textureID = 0;
    }

    texturePath = path;

    // Cargar nueva textura con sistema de referencias
    textureID = Texture::LoadTextureManaged(path);

    if (textureID != 0)
    {
        // Obtener info de la textura desde el cache
        TextureData texData = Texture::LoadTextureWithInfo(path);
        width = texData.width;
        height = texData.height;
        channels = texData.channels;

        if (channels == 4)
        {
            alphaMode = AlphaMode::ALPHA_TEST;
            alphaCutoff = 0.5f;
            std::cout << "[Material] Auto-enabled ALPHA_TEST" << std::endl;
        }
        else
        {
            alphaMode = AlphaMode::OPAQUE;
            std::cout << "[Material] Opaque texture" << std::endl;
        }
    }
    else
    {
        std::cerr << "[Material] Failed to load texture: " << path << std::endl;
    }
}

void ComponentMaterial::SetTexture(GLuint texID, const char* path, int texChannels)
{
    // Solo liberar si no es checkerboard
    if (textureID != 0 && !texturePath.empty() && texturePath != "checkerboard_default")
    {
        Texture::ReleaseTexture(texturePath);
    }

    textureID = texID;
    if (path && path[0] != '\0')
        texturePath = path;

    if (texChannels > 0)
    {
        channels = texChannels;

        if (channels == 4)
        {
            alphaMode = AlphaMode::ALPHA_TEST;
            alphaCutoff = 0.5f;
        }
        else
        {
            alphaMode = AlphaMode::OPAQUE;
        }
    }
}

void ComponentMaterial::SetOverrideTexture(GLuint texID, bool takeOwnership)
{
    ClearOverrideTexture();
    overrideTextureID = texID;
    overrideTextureOwned = takeOwnership;
}

void ComponentMaterial::ClearOverrideTexture()
{
    // Solo borrar la textura override si realmente la poseemos
    // El inspector puede establecer override con texturas checkerboard que no debemos borrar
    if (overrideTextureOwned && overrideTextureID != 0)
    {
        glDeleteTextures(1, &overrideTextureID);
    }
    overrideTextureID = 0;
    overrideTextureOwned = false;
}

void ComponentMaterial::Bind()
{
    GLuint texToBind = (overrideTextureID != 0) ? overrideTextureID : textureID;
    glBindTexture(GL_TEXTURE_2D, texToBind);

    if (alphaMode == AlphaMode::ALPHA_TEST)
    {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, alphaCutoff);
    }
    else
    {
        glDisable(GL_ALPHA_TEST);
    }

    if (alphaMode == AlphaMode::ALPHA_BLEND)
    {
        glEnable(GL_BLEND);
        SetupBlendMode();
    }
    else
    {
        glDisable(GL_BLEND);
    }
}

void ComponentMaterial::Unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
}

void ComponentMaterial::SetupBlendMode()
{
    switch (blendMode)
    {
    case BlendMode::STANDARD:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case BlendMode::ADDITIVE:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
    case BlendMode::MULTIPLY:
        glBlendFunc(GL_DST_COLOR, GL_ZERO);
        break;
    case BlendMode::SCREEN:
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
        break;
    case BlendMode::PREMULTIPLIED:
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        break;
    }
}

void ComponentMaterial::OnEditor()
{
    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Texture: %s", texturePath.c_str());
        ImGui::Text("Size: %dx%d", width, height);
        ImGui::Text("Channels: %d", channels);

        ImGui::Separator();

        const char* alphaModeNames[] = { "Opaque", "Alpha Test", "Alpha Blend" };
        int currentMode = (int)alphaMode;
        if (ImGui::Combo("Alpha Mode", &currentMode, alphaModeNames, 3))
        {
            alphaMode = (AlphaMode)currentMode;
        }

        if (alphaMode == AlphaMode::ALPHA_TEST)
        {
            ImGui::SliderFloat("Alpha Cutoff", &alphaCutoff, 0.0f, 1.0f);
        }

        if (alphaMode == AlphaMode::ALPHA_BLEND)
        {
            const char* blendModeNames[] = { "Standard", "Additive", "Multiply", "Screen", "Premultiplied" };
            int currentBlend = (int)blendMode;
            if (ImGui::Combo("Blend Mode", &currentBlend, blendModeNames, 5))
            {
                blendMode = (BlendMode)currentBlend;
            }

            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "Warning: Needs depth sorting!");
        }
    }
}

void ComponentMaterial::CleanUp()
{
    // Solo liberar texturas que NO sean checkerboard default
    // El checkerboard es una textura compartida que no debe liberarse
    if (textureID != 0 && !texturePath.empty() && texturePath != "checkerboard_default")
    {
        Texture::ReleaseTexture(texturePath);
        textureID = 0;
        texturePath.clear();
    }

    ClearOverrideTexture();
}