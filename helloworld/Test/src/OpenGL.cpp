#include "OpenGL.h"
#include "Application.h"
#include "Shader.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <iostream>
#include <IL/il.h>
#include <IL/ilu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define CHECKERS_WIDTH 64
#define CHECKERS_HEIGHT 64

OpenGL::OpenGL() : glContext(nullptr), VAO(0), VBO(0), texture(0), shader(nullptr), rotationAngle(0.0f) {}

OpenGL::~OpenGL()
{
    if (shader) delete shader;
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

    // Habilitar depth test
    glEnable(GL_DEPTH_TEST);

    // Inicializar DevIL
    ilInit();
    iluInit();

    // Shaders
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aColor;\n"
        "layout (location = 2) in vec2 aTexCoord;\n"
        "out vec3 ourColor;\n"
        "out vec2 TexCoord;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "    ourColor = aColor;\n"
        "    TexCoord = aTexCoord;\n"
        "}\0";

    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec3 ourColor;\n"
        "in vec2 TexCoord;\n"
        "uniform sampler2D texture1;\n"
        "void main()\n"
        "{\n"
        "    FragColor = texture(texture1, TexCoord) * vec4(ourColor, 1.0);\n"
        "}\0";

    shader = new Shader(vertexShaderSource, fragmentShaderSource);

    float pyramidVertices[] = {
        // Posición          // Color           // UV
        // Base
        -0.5f, 0.0f, -0.5f,  1,0,0,  0,0,
         0.5f, 0.0f, -0.5f,  0,1,0,  1,0,
         0.5f, 0.0f,  0.5f,  0,0,1,  1,1,
        -0.5f, 0.0f,  0.5f,  1,1,0,  0,1,
        // Vértice superior
         0.0f, 0.8f, 0.0f,   1,1,1,  0.5f,0.5f
    };

    unsigned int pyramidIndices[] = {
        // Base (2 triángulos para formar el cuadrado)
        0, 1, 2,
        0, 2, 3,
        // Caras laterales
        0, 1, 4,  // Frente
        1, 2, 4,  // Derecha
        2, 3, 4,  // Atrás
        3, 0, 4   // Izquierda
    };


    glGenVertexArrays(1, &pyramidVAO);
    glGenBuffers(1, &pyramidVBO);
    glGenBuffers(1, &pyramidEBO);

    glBindVertexArray(pyramidVAO);

    glBindBuffer(GL_ARRAY_BUFFER, pyramidVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVertices), pyramidVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pyramidEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(pyramidIndices), pyramidIndices, GL_STATIC_DRAW);

    // Posición
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // UV
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);


    texture = LoadTexture("../Assets/Textures/sigma.jpg");

    return true;
}

bool OpenGL::Update()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader->use();

    // Rotación continua
    rotationAngle += 0.01f; // Velocidad de giro
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.2f, -2.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    // CORRECCIÓN: Usar pyramidVAO y glDrawElements
    glBindVertexArray(pyramidVAO);
    glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);  // 18 índices (6 triángulos * 3 vértices)
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

unsigned int OpenGL::LoadTexture(const char* path)
{
    ILuint imgID;
    ilGenImages(1, &imgID);
    ilBindImage(imgID);

    std::cout << "Intentando cargar: " << path << std::endl;

    if (!ilLoadImage(path)) {
        ILenum error = ilGetError();
        std::cerr << "ERROR: No se pudo cargar la imagen. DevIL Code: "
            << error << " -> " << iluErrorString(error) << std::endl;
        ilDeleteImages(1, &imgID);
        return 0;
    }

    if (!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE)) {
        ILenum error = ilGetError();
        std::cerr << "ERROR: No se pudo convertir la imagen a RGBA: "
            << iluErrorString(error) << std::endl;
        ilDeleteImages(1, &imgID);
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        ilGetInteger(IL_IMAGE_WIDTH),
        ilGetInteger(IL_IMAGE_HEIGHT),
        0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    ilDeleteImages(1, &imgID);
    return texID;
}

bool OpenGL::CleanUp()
{
    glDeleteVertexArrays(1, &pyramidVAO);
    glDeleteBuffers(1, &pyramidVBO);
    glDeleteBuffers(1, &pyramidEBO);
    glDeleteTextures(1, &texture);

    if (shader) {
        glDeleteProgram(shader->ID);
        delete shader;
        shader = nullptr;
    }

    if (glContext) SDL_GL_DestroyContext(glContext);
    return true;
}