#include "ShaderManager.h"
#include "ShaderLibrary.h"
#include "ModuleEditor.h"

std::unique_ptr<Shader> ShaderManager::unlitShader = nullptr;
std::unique_ptr<Shader> ShaderManager::phongShader = nullptr;
bool ShaderManager::initialized = false;

bool ShaderManager::Initialize()
{
    if (initialized)
        return true;

    try
    {
        // Compilar shader UNLIT
        unlitShader = std::make_unique<Shader>(
            ShaderLibrary::unlitVertexShader,
            ShaderLibrary::unlitFragmentShader
        );
        ModuleEditor::PushEngineLog("[ShaderManager] Unlit shader compiled successfully");

        // Compilar shader PHONG
        phongShader = std::make_unique<Shader>(
            ShaderLibrary::phongVertexShader,
            ShaderLibrary::phongFragmentShader
        );
        ModuleEditor::PushEngineLog("[ShaderManager] Phong shader compiled successfully");

        initialized = true;
        return true;
    }
    catch (...)
    {
        ModuleEditor::PushEngineLog("[ShaderManager] ERROR: Failed to compile shaders");
        return false;
    }
}

void ShaderManager::Shutdown()
{
    unlitShader.reset();
    phongShader.reset();
    initialized = false;
    ModuleEditor::PushEngineLog("[ShaderManager] Shaders cleaned up");
}

Shader* ShaderManager::GetUnlitShader()
{
    return unlitShader.get();
}

Shader* ShaderManager::GetPhongShader()
{
    return phongShader.get();
}