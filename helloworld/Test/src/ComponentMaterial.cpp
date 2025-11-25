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
    CleanUp();
}

void ComponentMaterial::LoadTexture(const char* path)
{
    CleanUp();

    texturePath = path;

    TextureData texData = Texture::LoadTextureWithInfo(path);

    textureID = texData.id;
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

void ComponentMaterial::SetTexture(GLuint texID, const char* path, int texChannels)
{
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

        // === SHADER SELECTION ===
        ImGui::Text("Shader Settings:");
        const char* shaderTypeNames[] = {
            "Unlit (No Lighting)",
            "Phong (Vertex)",
            "Phong (Pixel)",
            "Blinn-Phong (Pixel)"
        };
        int currentShaderType = (int)shaderType;
        if (ImGui::Combo("Shader Type", &currentShaderType, shaderTypeNames, 4))
        {
            shaderType = (ShaderType)currentShaderType;
        }

        // Mostrar info sobre el shader seleccionado
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
        switch (shaderType)
        {
        case ShaderType::UNLIT:
            ImGui::TextWrapped("No lighting calculations. Shows texture as-is.");
            break;
        case ShaderType::VERTEX_PHONG:
            ImGui::TextWrapped("Phong lighting calculated per-vertex. Faster but less accurate.");
            break;
        case ShaderType::PIXEL_PHONG:
            ImGui::TextWrapped("Phong lighting calculated per-pixel. More accurate specular highlights.");
            break;
        case ShaderType::PIXEL_BLINN_PHONG:
            ImGui::TextWrapped("Blinn-Phong lighting per-pixel. Better performance than Phong with similar quality.");
            break;
        }
        ImGui::PopStyleColor();

        ImGui::Separator();

        // === LIGHTING PROPERTIES (solo si NO es UNLIT) ===
        if (shaderType != ShaderType::UNLIT)
        {
            ImGui::Text("Lighting Properties:");

            float ambient[3] = { ambientColor.x, ambientColor.y, ambientColor.z };
            if (ImGui::ColorEdit3("Ambient", ambient))
            {
                ambientColor = glm::vec3(ambient[0], ambient[1], ambient[2]);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Base color in shadows (minimum brightness)");

            float diffuse[3] = { diffuseColor.x, diffuseColor.y, diffuseColor.z };
            if (ImGui::ColorEdit3("Diffuse", diffuse))
            {
                diffuseColor = glm::vec3(diffuse[0], diffuse[1], diffuse[2]);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Main surface color affected by light direction");

            float specular[3] = { specularColor.x, specularColor.y, specularColor.z };
            if (ImGui::ColorEdit3("Specular", specular))
            {
                specularColor = glm::vec3(specular[0], specular[1], specular[2]);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Highlight color (shininess reflection)");

            ImGui::SliderFloat("Shininess", &shininess, 1.0f, 256.0f, "%.1f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Controls how sharp/focused the specular highlight is (higher = sharper)");

            ImGui::Separator();
        }

        // === COLOR TINT ===
        float tint[4] = { colorTint.x, colorTint.y, colorTint.z, colorTint.w };
        if (ImGui::ColorEdit4("Color Tint", tint))
        {
            colorTint = glm::vec4(tint[0], tint[1], tint[2], tint[3]);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Multiplies the texture color");

        ImGui::Separator();

        // === ALPHA MODE ===
        const char* alphaModeNames[] = { "Opaque", "Alpha Test", "Alpha Blend" };
        int currentMode = (int)alphaMode;
        if (ImGui::Combo("Alpha Mode", &currentMode, alphaModeNames, 3))
        {
            alphaMode = (AlphaMode)currentMode;
        }

        if (alphaMode == AlphaMode::ALPHA_TEST)
        {
            ImGui::SliderFloat("Alpha Cutoff", &alphaCutoff, 0.0f, 1.0f);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                "Pixels with alpha < %.2f will be discarded", alphaCutoff);
        }

        if (alphaMode == AlphaMode::ALPHA_BLEND)
        {
            const char* blendModeNames[] = { "Standard", "Additive", "Multiply", "Screen", "Premultiplied" };
            int currentBlend = (int)blendMode;
            if (ImGui::Combo("Blend Mode", &currentBlend, blendModeNames, 5))
            {
                blendMode = (BlendMode)currentBlend;
            }

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "Warning: Needs depth sorting!");
            ImGui::TextWrapped("Blended objects are rendered back-to-front automatically.");

            ImGui::Spacing();
            ImGui::Text("Blend Mode Info:");
            switch (blendMode)
            {
            case BlendMode::STANDARD:
                ImGui::BulletText("Standard transparency");
                ImGui::BulletText("Formula: SrcAlpha + (1-SrcAlpha)*Dst");
                break;
            case BlendMode::ADDITIVE:
                ImGui::BulletText("Additive blending (glow effect)");
                ImGui::BulletText("Formula: SrcAlpha*Src + Dst");
                break;
            case BlendMode::MULTIPLY:
                ImGui::BulletText("Multiply blending (darken)");
                ImGui::BulletText("Formula: Dst * Src");
                break;
            case BlendMode::SCREEN:
                ImGui::BulletText("Screen blending (lighten)");
                ImGui::BulletText("Formula: 1 - (1-Src)*(1-Dst)");
                break;
            case BlendMode::PREMULTIPLIED:
                ImGui::BulletText("Premultiplied alpha");
                ImGui::BulletText("Formula: Src + (1-SrcAlpha)*Dst");
                break;
            }
        }
    }
}

void ComponentMaterial::CleanUp()
{
    if (textureID != 0)
    {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }

    ClearOverrideTexture();
}