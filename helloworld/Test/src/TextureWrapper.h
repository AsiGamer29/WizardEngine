#pragma once
#include <glad/glad.h>

// Wrapper simple para usar GLuint como Texture en MaterialComponent
class TextureWrapper {
private:
    GLuint textureID;
    bool shouldDelete;

public:
    TextureWrapper(GLuint id, bool takeOwnership = false)
        : textureID(id), shouldDelete(takeOwnership) {
    }

    ~TextureWrapper() {
        if (shouldDelete && textureID != 0) {
            glDeleteTextures(1, &textureID);
        }
    }

    void bind(unsigned int unit = 0) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }

    void unbind() const {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    GLuint getID() const { return textureID; }
};