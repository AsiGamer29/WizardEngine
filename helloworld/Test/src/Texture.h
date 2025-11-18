#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>
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
};

#endif