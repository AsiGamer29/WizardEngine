#include "OpenGL.h"
#include "Application.h"
#include "Model.h"
#include "Texture.h"
#include "GeometryGenerator.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <iostream>
#include <IL/il.h>
#include <IL/ilu.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <set>

OpenGL::OpenGL()
    : glContext(nullptr), shader(nullptr), debugShader(nullptr), gridShader(nullptr),
    fbxModel(nullptr), rotationAngle(0.0f), texture(0),
    gridVAO(0), gridVBO(0), gridLineCount(0), showGrid(true),
    currentGeometry(nullptr), isGeometryActive(false) {
}

OpenGL::~OpenGL()
{
    if (shader) delete shader;
    if (debugShader) delete debugShader;
    if (gridShader) delete gridShader;
    if (fbxModel) delete fbxModel;
    if (currentGeometry) {
        currentGeometry->Cleanup();
        delete currentGeometry;
    }
}

void OpenGL::CreateGrid(int size)
{
    std::vector<float> gridVertices;

    for (int z = -size; z <= size; ++z) {
        gridVertices.push_back(-size);
        gridVertices.push_back(0.0f);
        gridVertices.push_back(z);

        gridVertices.push_back(size);
        gridVertices.push_back(0.0f);
        gridVertices.push_back(z);
    }

    for (int x = -size; x <= size; ++x) {
        gridVertices.push_back(x);
        gridVertices.push_back(0.0f);
        gridVertices.push_back(-size);

        gridVertices.push_back(x);
        gridVertices.push_back(0.0f);
        gridVertices.push_back(size);
    }

    gridLineCount = gridVertices.size() / 3;

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    std::cout << "Grid created with " << (size * 2 + 1) * 2 << " lines" << std::endl;
}

void OpenGL::DrawGrid()
{
    if (!showGrid || gridVAO == 0) return;

    gridShader->use();

    Application& app = Application::GetInstance();

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = app.camera->getViewMatrix();
    glm::mat4 projection = app.camera->getProjectionMatrix();

    glUniformMatrix4fv(glGetUniformLocation(gridShader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(gridShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(gridShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glm::vec3 gridColor(0.5f, 0.5f, 0.5f);
    glUniform3fv(glGetUniformLocation(gridShader->ID, "gridColor"), 1, glm::value_ptr(gridColor));

    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, gridLineCount);
    glBindVertexArray(0);
}

void OpenGL::DrawGameObjects(GameObject* go)
{
    if (!go || !go->IsActive())
        return;

    ComponentTransform* transform = go->GetComponent<ComponentTransform>();
    ComponentMesh* mesh = go->GetComponent<ComponentMesh>();
    ComponentMaterial* material = go->GetComponent<ComponentMaterial>();

    if (mesh && transform)
    {
        shader->use();

        Application& app = Application::GetInstance();

        glm::mat4 modelMatrix = transform->GetGlobalMatrix();
        glm::mat4 view = app.camera->getViewMatrix();
        glm::mat4 projection = app.camera->getProjectionMatrix();

        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glm::vec3 lightPos(2.0f, 2.0f, 2.0f);
        glm::vec3 viewPos = app.camera->getPosition();
        glm::vec3 lightColor(1.0f);

        glUniform3fv(glGetUniformLocation(shader->ID, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shader->ID, "viewPos"), 1, glm::value_ptr(viewPos));
        glUniform3fv(glGetUniformLocation(shader->ID, "lightColor"), 1, glm::value_ptr(lightColor));

        if (material)
        {
            material->Bind();
        }
        else
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);
        }

        mesh->Draw();

        if (app.moduleScene && app.moduleScene->GetDebugShowNormals())
        {
            if (debugShader)
            {
                debugShader->use();
                glUniformMatrix4fv(glGetUniformLocation(debugShader->ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
                glUniformMatrix4fv(glGetUniformLocation(debugShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
                glUniformMatrix4fv(glGetUniformLocation(debugShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
                glUniform3f(glGetUniformLocation(debugShader->ID, "color"), 1.0f, 0.0f, 0.0f);
            }
        }
    }

    for (GameObject* child : go->GetChildren())
    {
        DrawGameObjects(child);
    }
}

void OpenGL::LoadGeometry(const std::string& type) {
    if (currentGeometry) {
        currentGeometry->Cleanup();
        delete currentGeometry;
        currentGeometry = nullptr;
    }

    if (fbxModel) {
        delete fbxModel;
        fbxModel = nullptr;
    }

    Application::GetInstance().moduleScene->ClearScene();
    Application::GetInstance().moduleScene->Start();

    currentGeometry = new MeshGeometry();

    if (type == "Cube") {
        *currentGeometry = GeometryGenerator::CreateCube(2.0f);
    }
    else if (type == "Sphere") {
        *currentGeometry = GeometryGenerator::CreateSphere(1.0f, 32, 16);
    }
    else if (type == "Cylinder") {
        *currentGeometry = GeometryGenerator::CreateCylinder(1.0f, 2.0f, 32);
    }
    else if (type == "Pyramid") {
        *currentGeometry = GeometryGenerator::CreatePyramid(2.0f, 2.0f);
    }
    else if (type == "Plane") {
        *currentGeometry = GeometryGenerator::CreatePlane(5.0f, 5.0f);
    }

    isGeometryActive = true;

    if (texture) {
        glDeleteTextures(1, &texture);
    }
    texture = Texture::CreateCheckerboardTexture(512, 512, 32);

    std::cout << "Loaded geometry: " << type << std::endl;
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
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    ilInit();
    iluInit();

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

    const char* debugVert = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";
    const char* debugFrag = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 color;
        void main() {
            FragColor = vec4(color, 1.0);
        }
    )";
    debugShader = new Shader(debugVert, debugFrag);

    const char* gridVert = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";
    const char* gridFrag = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 gridColor;
        void main() {
            FragColor = vec4(gridColor, 1.0);
        }
    )";
    gridShader = new Shader(gridVert, gridFrag);
    CreateGrid(20);

    try
    {
        auto& app = Application::GetInstance();
        app.moduleScene->LoadModel("Assets/Models/BakerHouse.fbx");

        GameObject* root = app.moduleScene->GetRoot();
        if (!root)
        {
            std::cerr << "ERROR: Root GameObject is null!" << std::endl;
            return false;
        }

        if (!root->GetChildren().empty())
        {
            GameObject* bakerHouse = root->GetChildren().back();

            ComponentTransform* transform = bakerHouse->GetComponent<ComponentTransform>();
            if (transform)
            {
                transform->SetScale(glm::vec3(0.01f));

                glm::quat correction = glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0));
                transform->SetRotation(correction);

                std::cout << "BakerHouse scaled and rotated to correct orientation" << std::endl;
            }
            // Cargar textura y aplicarla
            GLuint bakerTexture = Texture::LoadTexture("Assets/Textures/Baker_house.png");
            if (!bakerTexture)
                bakerTexture = Texture::CreateCheckerboardTexture(512, 512, 32);

            ApplyTextureToGameObjects(bakerHouse, bakerTexture, "Assets/Textures/Baker_house.png");

            // Seleccionar el GameObject
            app.moduleScene->SetSelectedGameObject(bakerHouse);

            std::cout << "BakerHouse loaded, scaled, textured, and selected." << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "ERROR: Could not load BakerHouse - " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "UNKNOWN ERROR: Could not load BakerHouse" << std::endl;
    }

    isGeometryActive = false;

    std::cout << "OpenGL initialization complete" << std::endl;
    return true;
}

bool OpenGL::PreUpdate()
{
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return true;
}

// Función auxiliar para recolectar todas las texturas en uso
void OpenGL::CollectTexturesInUse(GameObject* go, std::set<GLuint>& texturesInUse)
{
    if (!go) return;

    ComponentMaterial* material = go->GetComponent<ComponentMaterial>();
    if (material)
    {
        GLuint texID = material->GetTextureID();
        if (texID != 0)
        {
            texturesInUse.insert(texID);
        }
    }

    for (GameObject* child : go->GetChildren())
    {
        CollectTexturesInUse(child, texturesInUse);
    }
}

bool OpenGL::Update()
{
    Application& app = Application::GetInstance();

    // Manejo de drag & drop
    if (!app.input->droppedFiles.empty())
    {
        for (const std::string& filePath : app.input->droppedFiles)
        {
            std::string ext = filePath.substr(filePath.find_last_of('.') + 1);
            for (auto& c : ext) c = tolower(c);

            if (ext == "fbx" || ext == "obj" || ext == "dae")
            {
                try
                {
                    std::cout << "=== LOADING MODEL: " << filePath << " ===" << std::endl;

                    // No limpiar la escena ni otros GameObjects
                    if (!app.moduleScene)
                    {
                        std::cerr << "ERROR: moduleScene is null!" << std::endl;
                        continue;
                    }

                    // Cargar modelo: ModuleScene se encarga de añadirlo al root
                    app.moduleScene->LoadModel(filePath.c_str());

                    GameObject* root = app.moduleScene->GetRoot();
                    if (!root || root->GetChildren().empty())
                    {
                        std::cerr << "ERROR: Failed to get root or no children after loading!" << std::endl;
                        continue;
                    }

                    // El modelo recién cargado será el último hijo del root
                    GameObject* newModel = root->GetChildren().back();
                    if (!newModel)
                    {
                        std::cerr << "ERROR: newModel is null!" << std::endl;
                        continue;
                    }

                    // Transformaciones: escalar y rotar
                    ComponentTransform* transform = newModel->GetComponent<ComponentTransform>();
                    if (transform)
                    {
                        transform->SetScale(glm::vec3(0.01f));
                        std::cout << "Applied scale and rotation to new model." << std::endl;
                    }

                    // Auto-seleccionar el modelo recién cargado
                    app.moduleScene->SetSelectedGameObject(newModel);
                    std::cout << "New model auto-selected: " << newModel->GetName() << std::endl;

                }
                catch (const std::exception& e)
                {
                    std::cerr << "EXCEPTION loading 3D model: " << e.what() << std::endl;
                }
                catch (...)
                {
                    std::cerr << "UNKNOWN EXCEPTION loading 3D model: " << filePath << std::endl;
                }
            }
            else if (ext == "jpg" || ext == "png" || ext == "tga" || ext == "bmp" || ext == "dds")
            {
                try
                {
                    std::cout << "=== LOADING TEXTURE: " << filePath << " ===" << std::endl;

                    GLuint newTex = (ext == "dds") ?
                        Texture::LoadDDSTexture(filePath.c_str()) :
                        Texture::LoadTexture(filePath.c_str());

                    if (!newTex || !glIsTexture(newTex))
                    {
                        std::cerr << "ERROR: Failed to load texture: " << filePath << std::endl;
                        continue;
                    }

                    // Recolectar texturas en uso antes
                    std::set<GLuint> texturesInUse;
                    GameObject* root = app.moduleScene->GetRoot();
                    if (root)
                        CollectTexturesInUse(root, texturesInUse);

                    // Aplicar textura al objeto seleccionado
                    GameObject* selected = app.moduleScene->GetSelectedGameObject();
                    if (selected)
                    {
                        ApplyTextureToGameObjects(selected, newTex, filePath.c_str());
                        std::cout << "Texture applied to selected object: " << selected->GetName() << std::endl;
                    }

                    // Recolectar texturas en uso después
                    std::set<GLuint> newTexturesInUse;
                    if (root)
                        CollectTexturesInUse(root, newTexturesInUse);

                    // Eliminar texturas antiguas no usadas
                    for (GLuint oldTex : texturesInUse)
                    {
                        if (newTexturesInUse.find(oldTex) == newTexturesInUse.end() &&
                            oldTex != texture &&
                            oldTex != newTex &&
                            glIsTexture(oldTex))
                        {
                            std::cout << "Deleting unused texture: " << oldTex << std::endl;
                            glDeleteTextures(1, &oldTex);
                        }
                    }

                    std::cout << "=== TEXTURE LOADING COMPLETE ===" << std::endl;
                }
                catch (const std::exception& e)
                {
                    std::cerr << "EXCEPTION loading texture: " << e.what() << std::endl;
                }
                catch (...)
                {
                    std::cerr << "UNKNOWN EXCEPTION loading texture: " << filePath << std::endl;
                }
            }
            else
            {
                std::cout << "Unsupported file format: " << filePath << std::endl;
            }
        }

        app.input->droppedFiles.clear();
    }

    // Draw Grid
    DrawGrid();

    if (!shader) return true;

    if (isGeometryActive && currentGeometry)
    {
        shader->use();

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::rotate(modelMatrix, rotationAngle, glm::vec3(0, 1, 0));

        glm::mat4 view = app.camera->getViewMatrix();
        glm::mat4 projection = app.camera->getProjectionMatrix();

        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glm::vec3 lightPos(2.0f, 2.0f, 2.0f);
        glm::vec3 viewPos = app.camera->getPosition();
        glm::vec3 lightColor(1.0f);

        glUniform3fv(glGetUniformLocation(shader->ID, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shader->ID, "viewPos"), 1, glm::value_ptr(viewPos));
        glUniform3fv(glGetUniformLocation(shader->ID, "lightColor"), 1, glm::value_ptr(lightColor));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        currentGeometry->Draw();
    }
    else
    {
        GameObject* root = app.moduleScene->GetRoot();
        if (root)
            DrawGameObjects(root);
    }

    return true;
}

void OpenGL::ApplyTextureToGameObjects(GameObject* go, GLuint texID, const char* path)
{
    if (!go)
    {
        std::cout << "ApplyTextureToGameObjects: GameObject is null" << std::endl;
        return;
    }

    try
    {
        std::cout << "Applying texture to GameObject: " << go->GetName() << std::endl;

        if (!glIsTexture(texID))
        {
            std::cerr << "ERROR: Invalid texture ID: " << texID << std::endl;
            return;
        }

        ComponentMaterial* material = go->GetComponent<ComponentMaterial>();
        if (material)
        {
            std::cout << "  - Setting texture on ComponentMaterial..." << std::endl;
            material->SetTexture(texID, path);
            std::cout << "  - Texture set successfully" << std::endl;
        }

        const std::vector<GameObject*>& children = go->GetChildren();
        std::cout << "  - Processing " << children.size() << " children" << std::endl;

        for (size_t i = 0; i < children.size(); ++i)
        {
            GameObject* child = children[i];
            if (child)
            {
                ApplyTextureToGameObjects(child, texID, path);
            }
            else
            {
                std::cerr << "WARNING: Child " << i << " is null!" << std::endl;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "EXCEPTION in ApplyTextureToGameObjects: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "UNKNOWN EXCEPTION in ApplyTextureToGameObjects" << std::endl;
    }
}

bool OpenGL::CleanUp()
{
    std::cout << "Cleaning up OpenGL resources..." << std::endl;

    Application::GetInstance().moduleScene->CleanUp();

    if (currentGeometry) {
        currentGeometry->Cleanup();
        delete currentGeometry;
        currentGeometry = nullptr;
    }

    if (gridVBO) {
        glDeleteBuffers(1, &gridVBO);
        gridVBO = 0;
    }
    if (gridVAO) {
        glDeleteVertexArrays(1, &gridVAO);
        gridVAO = 0;
    }

    if (texture && glIsTexture(texture))
    {
        glDeleteTextures(1, &texture);
        texture = 0;
    }

    if (shader)
    {
        delete shader;
        shader = nullptr;
    }

    if (debugShader)
    {
        delete debugShader;
        debugShader = nullptr;
    }

    if (gridShader)
    {
        delete gridShader;
        gridShader = nullptr;
    }

    if (glContext)
    {
        SDL_GL_DestroyContext(static_cast<SDL_GLContext>(glContext));
        glContext = nullptr;
    }

    std::cout << "OpenGL cleanup complete" << std::endl;
    return true;
}