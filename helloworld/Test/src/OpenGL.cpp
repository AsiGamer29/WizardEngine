#include "OpenGL.h"
#include "Application.h"
#include "Model.h"
#include "Texture.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <iostream>
#include <IL/il.h>
#include <IL/ilu.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

OpenGL::OpenGL()
    : glContext(nullptr), shader(nullptr), fbxModel(nullptr), rotationAngle(0.0f), texture(0) {
}

OpenGL::~OpenGL()
{
    if (shader) delete shader;
    if (fbxModel) delete fbxModel;
}

bool OpenGL::Start()
{
    SDL_Window* window = Application::GetInstance().window->GetWindow();
    glContext = SDL_GL_CreateContext(window);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glEnable(GL_DEPTH_TEST);

    // Initialize DevIL
    ilInit();
    iluInit();

    // Compile shaders
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;
        out vec2 TexCoord;
        out vec3 Normal;
        out vec3 FragPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main()
        {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            TexCoord = aTexCoord;
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;
        in vec3 Normal;
        in vec3 FragPos;
        uniform sampler2D texture_diffuse1;
        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform vec3 lightColor;
        void main()
        {
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * lightColor;
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * lightColor;
            vec4 texColor = texture(texture_diffuse1, TexCoord);
            if(texColor.a < 0.1) texColor = vec4(1.0);
            vec3 result = (ambient + diffuse + specular) * vec3(texColor);
            FragColor = vec4(result, 1.0);
        }
    )";

    shader = new Shader(vertexShaderSource, fragmentShaderSource);

    // Load default texture
    texture = Texture::LoadTexture("../Assets/Textures/wall.jpg");
    if (texture == 0)
    {
        std::cout << "Default texture not found, generating checkerboard..." << std::endl;
        texture = Texture::CreateCheckerboardTexture(512, 512, 32);
    }

    // Load default model
    try
    {
        fbxModel = new Model("../Assets/Models/BakerHouse.fbx");
        fbxModel->ClearTextures();
        std::cout << "Successfully loaded FBX model" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Warning: Could not load FBX model - " << e.what() << std::endl;
        fbxModel = nullptr;
    }

    std::cout << "OpenGL initialization complete" << std::endl;
    return true;
}

bool OpenGL::PreUpdate()
{
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    return true;
}

bool OpenGL::Update()
{
    Application& app = Application::GetInstance();

    // Handle drag & drop
    if (!app.input->droppedFiles.empty())
    {
        for (const std::string& filePath : app.input->droppedFiles)
        {
            std::string ext = filePath.substr(filePath.find_last_of('.') + 1);
            for (auto& c : ext) c = tolower(c);

            if (ext == "fbx" || ext == "obj" || ext == "dae")
            {
                if (fbxModel) { delete fbxModel; fbxModel = nullptr; }
                fbxModel = new Model(filePath);
                fbxModel->ClearTextures();

                if (texture)
                {
                    glDeleteTextures(1, &texture);
                    texture = Texture::CreateCheckerboardTexture(512, 512, 32);
                }

                std::cout << "Loaded 3D model: " << filePath << std::endl;
            }
            else if (ext == "jpg" || ext == "png" || ext == "tga" || ext == "bmp" || ext == "dds")
            {
                GLuint newTex = (ext == "dds") ?
                    Texture::LoadDDSTexture(filePath.c_str()) :
                    Texture::LoadTexture(filePath.c_str());

                if (newTex)
                {
                    if (texture)
                        glDeleteTextures(1, &texture);
                    texture = newTex;
                    std::cout << "Applied new texture: " << filePath << std::endl;
                }
            }
            else
            {
                std::cout << "Unsupported file format: " << filePath << std::endl;
            }
        }

        app.input->droppedFiles.clear();
    }

    // Renderiza tu modelo 3D
    if (!shader) return true;
    shader->use();

    // Camera setup
    view = app.camera->getViewMatrix();
    projection = app.camera->getProjectionMatrix();

    glm::vec3 lightPos(2.0f, 2.0f, 2.0f);
    glm::vec3 viewPos = app.camera->getPosition();
    glm::vec3 lightColor(1.0f);

    glUniform3fv(glGetUniformLocation(shader->ID, "lightPos"), 1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shader->ID, "viewPos"), 1, glm::value_ptr(viewPos));
    glUniform3fv(glGetUniformLocation(shader->ID, "lightColor"), 1, glm::value_ptr(lightColor));

    if (fbxModel)
    {
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::rotate(modelMatrix, rotationAngle, glm::vec3(0, 1, 0));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f));

        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        fbxModel->Draw(*shader);
    }

    return true;
}

bool OpenGL::CleanUp()
{
    std::cout << "Cleaning up OpenGL resources..." << std::endl;
    if (texture)
        glDeleteTextures(1, &texture);
    return true;
}