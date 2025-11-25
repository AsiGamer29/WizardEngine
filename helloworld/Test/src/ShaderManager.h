#pragma once
#include "Shader.h"
#include <memory>
#include <string>

class ShaderManager
{
private:
    static std::unique_ptr<Shader> unlitShader;
    static std::unique_ptr<Shader> phongShader;
    static bool initialized;

public:
    static bool Initialize();
    static void Shutdown();

    static Shader* GetUnlitShader();
    static Shader* GetPhongShader();
};