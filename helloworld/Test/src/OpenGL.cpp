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

    // Initialize DevIL image library
    ilInit();
    iluInit();

    // Vertex shader for 3D models with lighting
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

    // Fragment shader with Phong lighting
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

    // Try loading default texture, fallback to checkerboard if not found
    texture = LoadTexture("../Assets/Textures/wall.jpg");
    if (texture == 0)
    {
        std::cout << "Default texture not found, generating checkerboard pattern..." << std::endl;
        texture = CreateCheckerboardTexture(512, 512, 32);
    }

    // Load initial 3D model
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

bool OpenGL::Update()
{
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Application& app = Application::GetInstance();

    // Handle drag and drop for models and textures
    if (!app.input->droppedFiles.empty())
    {
        for (const std::string& filePath : app.input->droppedFiles)
        {
            std::string ext = filePath.substr(filePath.find_last_of('.') + 1);
            for (auto& c : ext) c = tolower(c);

            // Check if it's a 3D model file
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
                std::cout << "Loaded 3D model: " << filePath << std::endl;
            }
            // Check if it's a texture file
            else if (ext == "jpg" || ext == "png" || ext == "tga" || ext == "bmp" || ext == "dds")
            {
                GLuint newTex = 0;

                // Load texture based on format
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


    if (!shader) return true;

    shader->use();

    // Setup camera matrices
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

        // Bind global texture
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
            unsigned char color = isWhite ? 255 : 40;
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

    std::cout << "Generated checkerboard texture (" << width << "x" << height << ")" << std::endl;
    return texID;
}

// Load DDS compressed texture format
unsigned int OpenGL::LoadDDSTexture(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        std::cout << "Could not open DDS file: " << path << std::endl;
        return 0;
    }

    // Verify DDS magic number
    char magic[4];
    file.read(magic, 4);
    if (std::strncmp(magic, "DDS ", 4) != 0)
    {
        std::cout << "Invalid DDS file format: " << path << std::endl;
        file.close();
        return 0;
    }

    // Read DDS header (124 bytes)
    unsigned char header[124];
    file.read(reinterpret_cast<char*>(header), 124);

    unsigned int height = *(unsigned int*)&(header[8]);
    unsigned int width = *(unsigned int*)&(header[12]);
    unsigned int mipMapCount = *(unsigned int*)&(header[24]);
    unsigned int fourCC = *(unsigned int*)&(header[80]);

    // Default to at least one mipmap level
    if (mipMapCount == 0) mipMapCount = 1;

    // Determine compression format
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
        std::cout << "Unsupported DDS compression format (0x" << std::hex << fourCC << "): " << path << std::endl;
        file.close();
        return 0;
    }

    // Calculate total buffer size for all mipmap levels
    unsigned int bufsize = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;

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

    // Read compressed texture data
    unsigned char* buffer = new unsigned char[totalSize];
    file.read(reinterpret_cast<char*>(buffer), totalSize);
    file.close();

    // Create OpenGL texture
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    // Upload all mipmap levels
    unsigned int offset = 0;
    w = width;
    h = height;

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

    std::cout << "Loaded DDS texture: " << path << " (" << width << "x" << height << ", " << mipMapCount << " mipmaps)" << std::endl;
    return texID;
}

// Load texture using DevIL (supports PNG, JPG, TGA, BMP)
unsigned int OpenGL::LoadTexture(const char* path)
{
    ILuint imgID;
    ilGenImages(1, &imgID);
    ilBindImage(imgID);

    if (!ilLoadImage(path))
    {
        std::cout << "Could not load texture: " << path << " - Using fallback checkerboard" << std::endl;
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

    std::cout << "Loaded texture: " << path << std::endl;
    return texID;
}

bool OpenGL::CleanUp()
{
    std::cout << "Cleaning up OpenGL resources..." << std::endl;
    if (texture)
        glDeleteTextures(1, &texture);
    return true;
}