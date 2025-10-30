#include "Texture.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <GL/gl.h>

unsigned int Texture::CreateCheckerboardTexture(int width, int height, int cellSize)
{
    int numChannels = 3;
    std::vector<unsigned char> data(width * height * numChannels);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            bool isWhite = ((x / cellSize) % 2 == (y / cellSize) % 2);
            unsigned char color = isWhite ? 255 : 40;
            int index = (y * width + x) * numChannels;
            data[index + 0] = color;
            data[index + 1] = color;
            data[index + 2] = color;
        }
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
        GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::cout << "[Texture] Generated checkerboard (" << width << "x" << height << ")" << std::endl;
    return texID;
}

unsigned int Texture::LoadTexture(const char* path)
{
    ILuint imgID;
    ilGenImages(1, &imgID);
    ilBindImage(imgID);

    if (!ilLoadImage(path))
    {
        std::cerr << "[Texture] Failed to load: " << path << " -> using fallback checkerboard" << std::endl;
        ilDeleteImages(1, &imgID);
        return CreateCheckerboardTexture(512, 512, 32);
    }

    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        ilGetInteger(IL_IMAGE_WIDTH),
        ilGetInteger(IL_IMAGE_HEIGHT),
        0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    ilDeleteImages(1, &imgID);
    std::cout << "[Texture] Loaded: " << path << std::endl;
    return texID;
}

unsigned int Texture::LoadDDSTexture(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        std::cout << "Could not open DDS file: " << path << std::endl;
        return 0;
    }

    // Verify DDS magic number
    char magic[4];
    file.read(magic, 4);
    if (std::strncmp(magic, "DDS ", 4) != 0)
    {
        std::cout << "Invalid DDS file format: " << path << std::endl;
        file.close();
        return 0;
    }

    // Read DDS header (124 bytes)
    unsigned char header[124];
    file.read(reinterpret_cast<char*>(header), 124);

    unsigned int height = *(unsigned int*)&(header[8]);
    unsigned int width = *(unsigned int*)&(header[12]);
    unsigned int mipMapCount = *(unsigned int*)&(header[24]);
    unsigned int fourCC = *(unsigned int*)&(header[80]);

    // Default to at least one mipmap level
    if (mipMapCount == 0) mipMapCount = 1;

    // Determine compression format
    GLenum format;
    unsigned int blockSize;

    if (fourCC == 0x31545844) // DXT1
    {
        format = 0x83F1; // GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
        blockSize = 8;
    }
    else if (fourCC == 0x33545844) // DXT3
    {
        format = 0x83F2; // GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
        blockSize = 16;
    }
    else if (fourCC == 0x35545844) // DXT5
    {
        format = 0x83F3; // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
        blockSize = 16;
    }
    else
    {
        std::cout << "Unsupported DDS compression format (0x" << std::hex << fourCC << "): " << path << std::endl;
        file.close();
        return 0;
    }

    // Calculate total buffer size for all mipmap levels
    unsigned int bufsize = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;

    unsigned int totalSize = bufsize;
    unsigned int w = width / 2;
    unsigned int h = height / 2;
    for (unsigned int i = 1; i < mipMapCount; i++)
    {
        if (w == 0) w = 1;
        if (h == 0) h = 1;
        totalSize += ((w + 3) / 4) * ((h + 3) / 4) * blockSize;
        w /= 2;
        h /= 2;
    }

    // Read compressed texture data
    unsigned char* buffer = new unsigned char[totalSize];
    file.read(reinterpret_cast<char*>(buffer), totalSize);
    file.close();

    // Create OpenGL texture
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    // Upload all mipmap levels
    unsigned int offset = 0;
    w = width;
    h = height;

    for (unsigned int level = 0; level < mipMapCount; ++level)
    {
        if (w == 0) w = 1;
        if (h == 0) h = 1;

        unsigned int size = ((w + 3) / 4) * ((h + 3) / 4) * blockSize;
        glCompressedTexImage2D(GL_TEXTURE_2D, level, format, w, h, 0, size, buffer + offset);

        offset += size;
        w /= 2;
        h /= 2;
    }

    delete[] buffer;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipMapCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::cout << "Loaded DDS texture: " << path << " (" << width << "x" << height << ", " << mipMapCount << " mipmaps)" << std::endl;
    return texID;
}
