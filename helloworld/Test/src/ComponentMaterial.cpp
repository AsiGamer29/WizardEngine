#include "ComponentMaterial.h"
#include "GameObject.h"
#include "Texture.h" // Tu clase Texture existente
#include <IL/il.h>
#include <IL/ilu.h>
#include <iostream>

ComponentMaterial::ComponentMaterial(GameObject* owner)
    : Component(owner, ComponentType::MATERIAL),
    textureID(0), width(0), height(0), channels(0), overrideTextureID(0), overrideTextureOwned(false)
{
    // Crear textura checkerboard por defecto
    textureID = Texture::CreateCheckerboardTexture(512, 512, 32);
    texturePath = "checkerboard_default";
    width = 512;
    height = 512;
    channels = 3;
}

ComponentMaterial::~ComponentMaterial()
{
    CleanUp();
}

void ComponentMaterial::LoadTexture(const char* path)
{
    if (!path || strlen(path) == 0)
    {
        std::cerr << "[ComponentMaterial] Invalid texture path" << std::endl;
        return;
    }

    // Limpiar textura anterior
    if (textureID != 0)
    {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }

    // Detectar formato
    std::string pathStr(path);
    std::string ext = pathStr.substr(pathStr.find_last_of('.') + 1);
    for (auto& c : ext) c = tolower(c);

    // Cargar según formato
    if (ext == "dds")
    {
        textureID = Texture::LoadDDSTexture(path);
    }
    else
    {
        // Usar DevIL para otros formatos (jpg, png, tga, bmp, etc.)
        ILuint imgID;
        ilGenImages(1, &imgID);
        ilBindImage(imgID);

        if (!ilLoadImage(path))
        {
            std::cerr << "[ComponentMaterial] Failed to load: " << path << std::endl;
            ilDeleteImages(1, &imgID);

            // Usar checkerboard como fallback
            textureID = Texture::CreateCheckerboardTexture(512, 512, 32);
            texturePath = "checkerboard_fallback";
            width = 512;
            height = 512;
            channels = 3;
            return;
        }

        // Convertir a RGBA
        ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

        // Obtener información de la imagen
        width = ilGetInteger(IL_IMAGE_WIDTH);
        height = ilGetInteger(IL_IMAGE_HEIGHT);
        channels = ilGetInteger(IL_IMAGE_CHANNELS);

        // Crear textura OpenGL
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
            width, height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
        glGenerateMipmap(GL_TEXTURE_2D);

        // Parámetros de textura
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        ilDeleteImages(1, &imgID);
        std::cout << "[ComponentMaterial] Loaded texture: " << path
            << " (" << width << "x" << height << ")" << std::endl;
    }

    texturePath = path;
}

void ComponentMaterial::SetTexture(GLuint texID, const char* path)
{
    if (textureID != 0 && textureID != texID)
    {
        glDeleteTextures(1, &textureID);
    }

    textureID = texID;
    if (path && strlen(path) > 0)
        texturePath = path;
    else
        texturePath = "external_texture";
}

void ComponentMaterial::SetOverrideTexture(GLuint texID, bool takeOwnership)
{
    // Si ya hay una override, liberarla si la poseemos
    if (overrideTextureID != 0 && overrideTextureOwned)
    {
        if (glIsTexture(overrideTextureID))
            glDeleteTextures(1, &overrideTextureID);
    }

    overrideTextureID = texID;
    overrideTextureOwned = takeOwnership;
}

void ComponentMaterial::ClearOverrideTexture()
{
    if (overrideTextureID != 0 && overrideTextureOwned)
    {
        if (glIsTexture(overrideTextureID))
            glDeleteTextures(1, &overrideTextureID);
    }
    overrideTextureID = 0;
    overrideTextureOwned = false;
}

void ComponentMaterial::Bind()
{
    GLuint toBind = textureID;
    if (overrideTextureID != 0)
        toBind = overrideTextureID;

    if (toBind != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, toBind);
    }
}

void ComponentMaterial::Unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ComponentMaterial::OnEditor()
{
    // TODO: Implementar con ImGui cuando hagas el inspector
}

void ComponentMaterial::CleanUp()
{
    // Limpiar override primero
    if (overrideTextureID != 0 && overrideTextureOwned)
    {
        if (glIsTexture(overrideTextureID))
            glDeleteTextures(1, &overrideTextureID);
        overrideTextureID = 0;
        overrideTextureOwned = false;
    }

    if (textureID != 0)
    {
        if (glIsTexture(textureID))
            glDeleteTextures(1, &textureID);
        textureID = 0;
    }
}