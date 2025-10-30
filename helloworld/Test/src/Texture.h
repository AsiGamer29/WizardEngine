#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>
#include <glad/glad.h>
#include <IL/il.h>
#include <IL/ilu.h>

class Texture {
public:
    static unsigned int LoadTexture(const char* path);
    static unsigned int LoadDDSTexture(const char* path);
    static unsigned int CreateCheckerboardTexture(int width, int height, int cellSize);
};

#endif
