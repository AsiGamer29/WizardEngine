#include "ComponentMaterial.h"
#include "GameObject.h"
#include "Texture.h" // Tu clase Texture existente
#include <IL/il.h>
#include <IL/ilu.h>
#include <iostream>

ComponentMaterial::ComponentMaterial(GameObject* owner)
    : Component(owner, ComponentType::MATERIAL),
    textureID(0), width(0), height(0), channels(0)
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

void ComponentMaterial::Bind()
{
    if (textureID != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }
}

void ComponentMaterial::Unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ComponentMaterial::OnEditor()
{
    // TODO: Implementar con ImGui cuando hagas el inspector
    // ImGui::Text("Texture: %s", texturePath.c_str());
    // ImGui::Text("Size: %dx%d", width, height);
    // ImGui::Image((void*)(intptr_t)textureID, ImVec2(100, 100));
}

void ComponentMaterial::CleanUp()
{
    if (textureID != 0)
    {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
}