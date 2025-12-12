#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>
#include <unordered_map>
#include <glad/glad.h>
#include <IL/il.h>
#include <IL/ilu.h>

struct TextureData
{
    unsigned int id;
    int width;
    int height;
    int channels;
};

class Texture {
public:
    static unsigned int LoadTexture(const char* path);
    static TextureData LoadTextureWithInfo(const char* path);
    static unsigned int LoadDDSTexture(const char* path);
    static unsigned int CreateCheckerboardTexture(int width, int height, int cellSize);

    static unsigned int LoadTextureManaged(const std::string& path);
    static void ReleaseTexture(unsigned int textureID);
    static void ReleaseTexture(const std::string& path);
    static int GetReferenceCount(unsigned int textureID);
    static void ClearAllTextures();

private:
    struct TextureInfo
    {
        unsigned int id;
        std::string path;
        int referenceCount;
        int width;
        int height;
        int channels;
    };

    static std::unordered_map<std::string, TextureInfo> textureCache;
    static std::unordered_map<unsigned int, std::string> idToPath;
};

#endif