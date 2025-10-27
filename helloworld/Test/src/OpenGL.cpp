#include "OpenGL.h"
#include "Application.h"
#include "Model.h"  // Incluir después de OpenGL.h
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <iostream>
#include <IL/il.h>
#include <IL/ilu.h>

OpenGL::OpenGL() : glContext(nullptr), shader(nullptr), fbxModel(nullptr), rotationAngle(0.0f) {}

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

    // Inicializar DevIL
    ilInit();
    iluInit();

    // === SHADERS PARA MODELOS 3D ===
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aNormal;\n"
        "layout (location = 2) in vec2 aTexCoord;\n"
        "out vec2 TexCoord;\n"
        "out vec3 Normal;\n"
        "out vec3 FragPos;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main()\n"
        "{\n"
        "    FragPos = vec3(model * vec4(aPos, 1.0));\n"
        "    Normal = mat3(transpose(inverse(model))) * aNormal;\n"
        "    TexCoord = aTexCoord;\n"
        "    gl_Position = projection * view * vec4(FragPos, 1.0);\n"
        "}\0";

    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoord;\n"
        "in vec3 Normal;\n"
        "in vec3 FragPos;\n"
        "uniform sampler2D texture_diffuse1;\n"
        "uniform vec3 lightPos;\n"
        "uniform vec3 viewPos;\n"
        "uniform vec3 lightColor;\n"
        "void main()\n"
        "{\n"
        "    float ambientStrength = 0.3;\n"
        "    vec3 ambient = ambientStrength * lightColor;\n"
        "    \n"
        "    vec3 norm = normalize(Normal);\n"
        "    vec3 lightDir = normalize(lightPos - FragPos);\n"
        "    float diff = max(dot(norm, lightDir), 0.0);\n"
        "    vec3 diffuse = diff * lightColor;\n"
        "    \n"
        "    float specularStrength = 0.5;\n"
        "    vec3 viewDir = normalize(viewPos - FragPos);\n"
        "    vec3 reflectDir = reflect(-lightDir, norm);\n"
        "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
        "    vec3 specular = specularStrength * spec * lightColor;\n"
        "    \n"
        "    vec4 texColor = texture(texture_diffuse1, TexCoord);\n"
        "    if(texColor.a < 0.1) texColor = vec4(1.0);\n"
        "    \n"
        "    vec3 result = (ambient + diffuse + specular) * vec3(texColor);\n"
        "    FragColor = vec4(result, 1.0);\n"
        "}\0";

    shader = new Shader(vertexShaderSource, fragmentShaderSource);

    // Cargar textura
    texture = LoadTexture("../Assets/Textures/wall.jpg");

    // Intentar cargar modelo FBX (opcional)
    try
    {
        fbxModel = new Model("../Assets/Models/backpack.fbx");
        std::cout << "Modelo FBX cargado correctamente" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Advertencia: No se pudo cargar modelo FBX: " << e.what() << std::endl;
        fbxModel = nullptr;
    }

    return true;
}

bool OpenGL::Update()
{
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!shader) return true;

    shader->use();

    Application& app = Application::GetInstance();

    // Usar matrices de la cámara
    view = app.camera->getViewMatrix();
    projection = app.camera->getProjectionMatrix();

    glm::vec3 lightPos(2.0f, 2.0f, 2.0f);
    glm::vec3 viewPos = app.camera->getPosition();
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

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

        fbxModel->Draw(*shader);
    }

    return true;
}


bool OpenGL::CleanUp()
{
    if (fbxModel)
    {
        delete fbxModel;
        fbxModel = nullptr;
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &pyramidVAO);
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteVertexArrays(1, &cylinderVAO);

    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &pyramidVBO);
    glDeleteBuffers(1, &pyramidEBO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
    glDeleteBuffers(1, &cylinderVBO);
    glDeleteBuffers(1, &cylinderEBO);

    glDeleteTextures(1, &texture);

    if (shader)
    {
        glDeleteProgram(shader->ID);
        delete shader;
        shader = nullptr;
    }

    if (glContext) SDL_GL_DestroyContext(glContext);
    return true;
}

unsigned int OpenGL::LoadTexture(const char* path)
{
    ILuint imgID;
    ilGenImages(1, &imgID);
    ilBindImage(imgID);

    std::cout << "Intentando cargar: " << path << std::endl;

    if (!ilLoadImage(path))
    {
        ILenum error = ilGetError();
        std::cerr << "ERROR: No se pudo cargar la imagen. DevIL Code: "
            << error << " -> " << iluErrorString(error) << std::endl;
        ilDeleteImages(1, &imgID);
        return 0;
    }

    if (!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE))
    {
        ILenum error = ilGetError();
        std::cerr << "ERROR: No se pudo convertir la imagen a RGBA: "
            << iluErrorString(error) << std::endl;
        ilDeleteImages(1, &imgID);
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ilGetInteger(IL_IMAGE_WIDTH),
        ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    ilDeleteImages(1, &imgID);
    return texID;
}