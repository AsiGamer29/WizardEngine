#pragma once
#include <string>

namespace ShaderLibrary
{
    // ========================================
    // SHADER BASICO SIN ILUMINACION (UNLIT)
    // ========================================
    const char* unlitVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

    const char* unlitFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;
uniform vec4 colorTint;
uniform float alphaCutoff;
uniform bool useAlphaTest;

void main()
{
    vec4 texColor = texture(texture1, TexCoord) * colorTint;
    
    if (useAlphaTest && texColor.a < alphaCutoff)
        discard;
    
    FragColor = texColor;
}
)";

    // ========================================
    // SHADER PHONG (VERTEX Y PIXEL)
    // ========================================
    const char* phongVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;
out vec3 VertexColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform float shininess;
uniform vec3 viewPos;
uniform bool doVertexLighting;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = normalMatrix * aNormal;
    TexCoord = aTexCoord;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
    
    if (doVertexLighting)
    {
        vec3 norm = normalize(Normal);
        vec3 lightDirection = normalize(lightDir);
        
        float diff = max(dot(norm, lightDirection), 0.0);
        
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDirection, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        
        vec3 ambient = ambientColor;
        vec3 diffuse = diff * diffuseColor * lightColor;
        vec3 specular = spec * specularColor * lightColor;
        
        VertexColor = ambient + diffuse + specular;
    }
    else
    {
        VertexColor = vec3(1.0);
    }
}
)";

    const char* phongFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;
in vec3 VertexColor;

uniform sampler2D texture1;
uniform vec4 colorTint;
uniform float alphaCutoff;
uniform bool useAlphaTest;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform float shininess;
uniform vec3 viewPos;

uniform bool doVertexLighting;
uniform bool useBlinnPhong;

void main()
{
    vec4 texColor = texture(texture1, TexCoord) * colorTint;
    
    if (useAlphaTest && texColor.a < alphaCutoff)
        discard;
    
    vec3 finalColor;
    
    if (doVertexLighting)
    {
        finalColor = VertexColor * texColor.rgb;
    }
    else
    {
        vec3 norm = normalize(Normal);
        vec3 lightDirection = normalize(lightDir);
        
        float diff = max(dot(norm, lightDirection), 0.0);
        
        vec3 viewDir = normalize(viewPos - FragPos);
        float spec = 0.0;
        
        if (useBlinnPhong)
        {
            vec3 halfwayDir = normalize(lightDirection + viewDir);
            spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
        }
        else
        {
            vec3 reflectDir = reflect(-lightDirection, norm);
            spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        }
        
        vec3 ambient = ambientColor;
        vec3 diffuse = diff * diffuseColor * lightColor;
        vec3 specular = spec * specularColor * lightColor;
        
        finalColor = (ambient + diffuse + specular) * texColor.rgb;
    }
    
    FragColor = vec4(finalColor, texColor.a);
}
)";
}