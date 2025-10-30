#include "OpenGL.h"
#include "Application.h"
#include "Model.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <iostream>
#include <IL/il.h>
#include <IL/ilu.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <cstring>

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
        "    vec3 norm = normalize(Normal);\n"
        "    vec3 lightDir = normalize(lightPos - FragPos);\n"
        "    float diff = max(dot(norm, lightDir), 0.0);\n"
        "    vec3 diffuse = diff * lightColor;\n"
        "    float specularStrength = 0.5;\n"
        "    vec3 viewDir = normalize(viewPos - FragPos);\n"
        "    vec3 reflectDir = reflect(-lightDir, norm);\n"
        "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
        "    vec3 specular = specularStrength * spec * lightColor;\n"
        "    vec4 texColor = texture(texture_diffuse1, TexCoord);\n"
        "    if(texColor.a < 0.1) texColor = vec4(1.0);\n"
        "    vec3 result = (ambient + diffuse + specular) * vec3(texColor);\n"
        "    FragColor = vec4(result, 1.0);\n"
        "}\0";

    shader = new Shader(vertexShaderSource, fragmentShaderSource);

    // === CHECKERBOARD / TEXTURA BASE ===
    texture = LoadTexture("../Assets/Textures/wall.jpg");
    if (texture == 0)
    {
        std::cout << "Texture not found! Generating debug texture..." << std::endl;
        texture = CreateCheckerboardTexture(512, 512, 32);
    }

    // === Cargar modelo inicial (sin texturas integradas) ===
    try
    {
        fbxModel = new Model("../Assets/Models/BakerHouse.fbx");
        fbxModel->ClearTextures();
        std::cout << "Fbx file loaded!" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Warning: FBX file failed to load!: " << e.what() << std::endl;
        fbxModel = nullptr;
    }

    std::cout << "OpenGL started successfully." << std::endl;
    return true;
}

bool OpenGL::Update()
{
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Application& app = Application::GetInstance();

    // --- Drag & Drop: cargar modelos o texturas ---
    if (!app.input->droppedFiles.empty())
    {
        for (const std::string& filePath : app.input->droppedFiles)
        {
            std::string ext = filePath.substr(filePath.find_last_of('.') + 1);
            for (auto& c : ext) c = tolower(c);

            if (ext == "fbx" || ext == "obj" || ext == "dae")
            {
                if (fbxModel)
                {
                    delete fbxModel;
                    fbxModel = nullptr;
                }

                fbxModel = new Model(filePath);
                fbxModel->ClearTextures();

                if (texture)
                {
                    glDeleteTextures(1, &texture);
                    texture = 0;
                }

                texture = CreateCheckerboardTexture(512, 512, 32);
                std::cout << "Modelo cargado con textura debug: " << filePath << std::endl;
            }
            else if (ext == "jpg" || ext == "png" || ext == "tga" || ext == "bmp" || ext == "dds")
            {
                GLuint newTex = 0;

                // Cargar según la extensión
                if (ext == "dds")
                {
                    newTex = LoadDDSTexture(filePath.c_str());
                }
                else
                {
                    newTex = LoadTexture(filePath.c_str());
                }

                if (newTex)
                {
                    if (texture)
                        glDeleteTextures(1, &texture);
                    texture = newTex;
                    std::cout << "Textura activa cambiada a: " << filePath << std::endl;
                }
            }
            else
            {
                std::cout << "Archivo no soportado: " << filePath << std::endl;
            }
        }

        app.input->droppedFiles.clear();
    }


    if (!shader) return true;

    shader->use();

    // Configurar matrices de cámara
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

        // === Aplicar SIEMPRE la textura global ===
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        fbxModel->Draw(*shader);
    }

    return true;
}

unsigned int OpenGL::CreateCheckerboardTexture(int width, int height, int cellSize)
{
    int numChannels = 3;
    std::vector<unsigned char> data(width * height * numChannels);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            bool isWhite = ((x / cellSize) % 2 == (y / cellSize) % 2);
            unsigned char color = isWhite ? 255 : 40; // blanco / gris oscuro
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

    std::cout << "Debug texture Checkerboard loaded (" << width << "x" << height << ")\n";
    return texID;
}

// Función para cargar DDS
unsigned int OpenGL::LoadDDSTexture(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        std::cout << "Failed to open DDS file: " << path << std::endl;
        return 0;
    }

    // Leer el magic number "DDS "
    char magic[4];
    file.read(magic, 4);
    if (std::strncmp(magic, "DDS ", 4) != 0)
    {
        std::cout << "Invalid DDS file: " << path << std::endl;
        file.close();
        return 0;
    }

    // Leer el header DDS (124 bytes)
    unsigned char header[124];
    file.read(reinterpret_cast<char*>(header), 124);

    unsigned int height = *(unsigned int*)&(header[8]);
    unsigned int width = *(unsigned int*)&(header[12]);
    unsigned int mipMapCount = *(unsigned int*)&(header[24]);
    unsigned int fourCC = *(unsigned int*)&(header[80]);

    // Si no hay mipmaps, establecer a 1
    if (mipMapCount == 0) mipMapCount = 1;

    // Determinar formato
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
        std::cout << "Unsupported DDS format (FourCC: 0x" << std::hex << fourCC << ") in: " << path << std::endl;
        file.close();
        return 0;
    }

    // Calcular tamaño total de datos
    unsigned int bufsize = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;

    // Para múltiples mipmaps, necesitamos más espacio
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

    unsigned char* buffer = new unsigned char[totalSize];
    file.read(reinterpret_cast<char*>(buffer), totalSize);
    file.close();

    // Crear textura OpenGL
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    unsigned int offset = 0;
    w = width;
    h = height;

    // Cargar mipmaps
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

    std::cout << "DDS Texture loaded: " << path << " (" << width << "x" << height << ", " << mipMapCount << " mipmaps)\n";
    return texID;
}

// Carga una textura desde archivo con DevIL
unsigned int OpenGL::LoadTexture(const char* path)
{
    ILuint imgID;
    ilGenImages(1, &imgID);
    ilBindImage(imgID);

    if (!ilLoadImage(path))
    {
        std::cout << "Texture failed to load: " << path
            << " — generating debug texture.\n";
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

    std::cout << "Texture loaded: " << path << "\n";
    return texID;
}

bool OpenGL::CleanUp()
{
    std::cout << "Cleaning OpenGL resources..." << std::endl;
    if (texture)
        glDeleteTextures(1, &texture);
    return true;
}