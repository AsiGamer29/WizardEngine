#include "OpenGL.h"
#include "Application.h"
#include "Input.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

OpenGL::OpenGL() : glContext(nullptr), shaderProgram(0), VAO(0), VBO(0), EBO(0)
{
    std::cout << "OpenGL Constructor" << std::endl;
}

OpenGL::~OpenGL()
{
}

bool OpenGL::Start()
{
    std::cout << "Init OpenGL Context & GLAD" << std::endl;

    SDL_Window* window = Application::GetInstance().window->GetWindow();
    glContext = SDL_GL_CreateContext(window);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glEnable(GL_DEPTH_TEST);

    // ---------- SHADERS ----------
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aColor;\n"
        "out vec3 vertexColor;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "   vertexColor = aColor;\n"
        "}\0";

    const char* fragmentShaderSource = "#version 330 core\n"
        "in vec3 vertexColor;\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(vertexColor, 1.0);\n"
        "}\0";

    // ---------- COMPILAR SHADERS ----------
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Vertex Shader Compilation Failed\n" << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Fragment Shader Compilation Failed\n" << infoLog << std::endl;
    }

    // ---------- LINK ----------
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR: Shader Program Linking Failed\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // ---------- VÉRTICES DEL CUBO ----------
    float vertices[] = {
        // posiciones           // colores
        -0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,  // 0
         0.5f, -0.5f, -0.5f,    0.0f, 1.0f, 0.0f,  // 1
         0.5f,  0.5f, -0.5f,    0.0f, 0.0f, 1.0f,  // 2
        -0.5f,  0.5f, -0.5f,    1.0f, 1.0f, 0.0f,  // 3
        -0.5f, -0.5f,  0.5f,    1.0f, 0.0f, 1.0f,  // 4
         0.5f, -0.5f,  0.5f,    0.0f, 1.0f, 1.0f,  // 5
         0.5f,  0.5f,  0.5f,    1.0f, 1.0f, 1.0f,  // 6
        -0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 0.0f   // 7
    };

    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,   // atrás
        4, 5, 6, 6, 7, 4,   // frente
        0, 4, 7, 7, 3, 0,   // izquierda
        1, 5, 6, 6, 2, 1,   // derecha
        3, 2, 6, 6, 7, 3,   // arriba
        0, 1, 5, 5, 4, 0    // abajo
    };

    // ---------- BUFFERS ----------
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Posición
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    std::cout << "OpenGL initialized successfully (3D mode)" << std::endl;



    return true;
}

bool OpenGL::Update()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    // ---------- MATRICES ----------
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, (float)SDL_GetTicks() / 1000.0f, glm::vec3(0.5f, 1.0f, 0.0f));

    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));

    glm::mat4 projection = glm::perspective(glm::radians(45.0f),
        800.0f / 600.0f, 0.1f, 100.0f);

    // Enviar matrices al shader
    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // ---------- DIBUJAR ----------
    
    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(pyramidVAO);
    glDrawElements(GL_TRIANGLES, pyramidIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(cylinderVAO);
    glDrawElements(GL_TRIANGLES, cylinderIndexCount, GL_UNSIGNED_INT, 0);


	GenerateShape();

    return true;
}

void OpenGL::GenerateShape()
{
    if (Application::GetInstance().input->GetKey(SDL_SCANCODE_1) == KEY_DOWN){
        GenerateSphere(0.75f, 20, 20); // radius 0.75, 20 stacks, 20 slices
    }
    else if (Application::GetInstance().input->GetKey(SDL_SCANCODE_2) == KEY_DOWN) {
        GeneratePyramid(1.0f, 1.0f); // base 1.0 and height 1.0
    }
    else if (Application::GetInstance().input->GetKey(SDL_SCANCODE_3) == KEY_DOWN) {
        GenerateCylinder(0.5f, 1.5f, 36); // radius, height, sectors
    }
}

void OpenGL::GenerateSphere(float radius, unsigned int stacks, unsigned int slices)
{
    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
    };

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Crear vértices
    for (unsigned int i = 0; i <= stacks; ++i)
    {
        float phi = glm::pi<float>() * float(i) / float(stacks);
        for (unsigned int j = 0; j <= slices; ++j)
        {
            float theta = 2.0f * glm::pi<float>() * float(j) / float(slices);

            Vertex v;
            v.position.x = radius * sin(phi) * cos(theta);
            v.position.y = radius * cos(phi);
            v.position.z = radius * sin(phi) * sin(theta);

            // Color basado en la posición
            v.color = glm::vec3((v.position.x + radius) / (2 * radius),
                (v.position.y + radius) / (2 * radius),
                (v.position.z + radius) / (2 * radius));

            vertices.push_back(v);
        }
    }

    // Crear índices
    for (unsigned int i = 0; i < stacks; ++i)
    {
        for (unsigned int j = 0; j < slices; ++j)
        {
            unsigned int first = i * (slices + 1) + j;
            unsigned int second = first + slices + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    sphereIndexCount = indices.size();

    // Crear buffers
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Posición
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void OpenGL::GeneratePyramid(float baseSize, float height)
{
    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
    };

    float half = baseSize / 2.0f;

    std::vector<Vertex> vertices = {
        // Base
        {{-half, 0.0f, -half}, {1.0f, 0.0f, 0.0f}}, // 0
        {{ half, 0.0f, -half}, {0.0f, 1.0f, 0.0f}}, // 1
        {{ half, 0.0f,  half}, {0.0f, 0.0f, 1.0f}}, // 2
        {{-half, 0.0f,  half}, {1.0f, 1.0f, 0.0f}}, // 3
        // Punta
        {{ 0.0f, height, 0.0f}, {1.0f, 0.0f, 1.0f}}  // 4
    };

    std::vector<unsigned int> indices = {
        // Base (dos triángulos)
        0, 1, 2,
        2, 3, 0,

        // Caras laterales
        0, 1, 4,
        1, 2, 4,
        2, 3, 4,
        3, 0, 4
    };

    pyramidIndexCount = indices.size();

    glGenVertexArrays(1, &pyramidVAO);
    glGenBuffers(1, &pyramidVBO);
    glGenBuffers(1, &pyramidEBO);

    glBindVertexArray(pyramidVAO);

    glBindBuffer(GL_ARRAY_BUFFER, pyramidVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pyramidEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Posición
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void OpenGL::GenerateCylinder(float radius, float height, int sectors)
{
    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
    };

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float halfHeight = height / 2.0f;
    float sectorStep = 2 * glm::pi<float>() / sectors;

    // Generar los vértices laterales
    for (int i = 0; i <= sectors; ++i)
    {
        float angle = i * sectorStep;
        float x = radius * cos(angle);
        float z = radius * sin(angle);

        glm::vec3 color = {
            (cos(angle) + 1.0f) * 0.5f,
            (sin(angle) + 1.0f) * 0.5f,
            1.0f - fabs(sin(angle))
        };

        // Vértice superior
        vertices.push_back({ glm::vec3(x, halfHeight, z), color });

        // Vértice inferior
        vertices.push_back({ glm::vec3(x, -halfHeight, z), color });
    }

    // Índices para los lados
    for (int i = 0; i < sectors; ++i)
    {
        int top1 = i * 2;
        int bottom1 = top1 + 1;
        int top2 = ((i + 1) % (sectors + 1)) * 2;
        int bottom2 = top2 + 1;

        // Primer triángulo
        indices.push_back(top1);
        indices.push_back(bottom1);
        indices.push_back(top2);

        // Segundo triángulo
        indices.push_back(bottom1);
        indices.push_back(bottom2);
        indices.push_back(top2);
    }

    // Centro de los discos
    int topCenterIndex = vertices.size();
    vertices.push_back({ glm::vec3(0.0f, halfHeight, 0.0f), {1.0f, 1.0f, 1.0f} });

    int bottomCenterIndex = vertices.size();
    vertices.push_back({ glm::vec3(0.0f, -halfHeight, 0.0f), {0.5f, 0.5f, 0.5f} });

    // Tapa superior
    for (int i = 0; i < sectors; ++i)
    {
        int next = (i + 1) % (sectors + 1);
        indices.push_back(topCenterIndex);
        indices.push_back(i * 2);
        indices.push_back(next * 2);
    }

    // Tapa inferior
    for (int i = 0; i < sectors; ++i)
    {
        int next = (i + 1) % (sectors + 1);
        indices.push_back(bottomCenterIndex);
        indices.push_back(next * 2 + 1);
        indices.push_back(i * 2 + 1);
    }

    cylinderIndexCount = indices.size();

    // ---------- BUFFER SETUP ----------
    glGenVertexArrays(1, &cylinderVAO);
    glGenBuffers(1, &cylinderVBO);
    glGenBuffers(1, &cylinderEBO);

    glBindVertexArray(cylinderVAO);

    glBindBuffer(GL_ARRAY_BUFFER, cylinderVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cylinderEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Posición
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}


bool OpenGL::CleanUp()
{
    std::cout << "Destroying OpenGL Context" << std::endl;

    glDeleteProgram(shaderProgram);

    if (glContext != nullptr)
    {
        SDL_GL_DestroyContext(glContext);
        glContext = nullptr;
    }

    return true;
}

void OpenGL::CleanUpSphere() {
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
}

void OpenGL::CleanUpPyramid() {
    glDeleteVertexArrays(1, &pyramidVAO);
    glDeleteBuffers(1, &pyramidVBO);
    glDeleteBuffers(1, &pyramidEBO);
}

void OpenGL::CleanUpCylinder() {
    glDeleteVertexArrays(1, &cylinderVAO);
    glDeleteBuffers(1, &cylinderVBO);
    glDeleteBuffers(1, &cylinderEBO);
}
       
