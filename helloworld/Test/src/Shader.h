#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
//#include <string>
#include <iostream>

class Shader
{
public:
    unsigned int ID;

    // Constructor compiles shaders from source code strings
    Shader(const char* vertexSource, const char* fragmentSource);

    // Use/activate the shader
    void use();

    // Utility uniform functions
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    void setVec4(const std::string& name, float x, float y, float z, float w) const;

private:
    // Utility function for checking shader compilation/linking errors
    void checkCompileErrors(unsigned int shader, std::string type);
};

#endif