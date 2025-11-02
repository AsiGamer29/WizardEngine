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
#include <glm/gtc/quaternion.hpp>  // ? Este sí que lo tienes (glm.lib incluye quaternion)
#include <vector>

OpenGL::OpenGL()
    : glContext(nullptr), shader(nullptr), debugShader(nullptr), fbxModel(nullptr), rotationAngle(0.0f), texture(0),
    gridVAO(0), gridVBO(0), gridLineCount(0), showGrid(true),
    currentGeometry(nullptr), isGeometryActive(false) {
}

OpenGL::~OpenGL()
{
    if (shader) delete shader;
    if (debugShader) delete debugShader;
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

    shader->use();

    Application& app = Application::GetInstance();

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = app.camera->getViewMatrix();
    glm::mat4 projection = app.camera->getProjectionMatrix();

    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glm::vec3 lightPos(0.0f, 50.0f, 0.0f);
    glm::vec3 viewPos = app.camera->getPosition();
    glm::vec3 lightColor(2.0f, 2.0f, 2.0f);

    glUniform3fv(glGetUniformLocation(shader->ID, "lightPos"), 1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shader->ID, "viewPos"), 1, glm::value_ptr(viewPos));
    glUniform3fv(glGetUniformLocation(shader->ID, "lightColor"), 1, glm::value_ptr(lightColor));

    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, gridLineCount);
    glBindVertexArray(0);
}

void OpenGL::DrawGameObjects(GameObject* go)
{
    if (!go || !go->IsActive())
        return;

    // Obtener componentes
    ComponentTransform* transform = go->GetComponent<ComponentTransform>();
    ComponentMesh* mesh = go->GetComponent<ComponentMesh>();
    ComponentMaterial* material = go->GetComponent<ComponentMaterial>();

    // Si tiene mesh, dibujarlo
    if (mesh && transform)
    {
        shader->use();

        Application& app = Application::GetInstance();

        // Obtener matriz global del transform (SIN rotación automática)
        glm::mat4 modelMatrix = transform->GetGlobalMatrix();

        glm::mat4 view = app.camera->getViewMatrix();
        glm::mat4 projection = app.camera->getProjectionMatrix();

        // Pasar matrices al shader
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Lighting
        glm::vec3 lightPos(2.0f, 2.0f, 2.0f);
        glm::vec3 viewPos = app.camera->getPosition();
        glm::vec3 lightColor(1.0f);

        glUniform3fv(glGetUniformLocation(shader->ID, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shader->ID, "viewPos"), 1, glm::value_ptr(viewPos));
        glUniform3fv(glGetUniformLocation(shader->ID, "lightColor"), 1, glm::value_ptr(lightColor));

        // Bind texture
        if (material)
        {
            material->Bind();
        }
        else
        {
            // Usar textura por defecto
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);
        }

        // Dibujar mesh
        mesh->Draw();

        // Si el editor pidió mostrar normales, dibujarlas usando debugShader
        if (app.moduleScene && app.moduleScene->GetDebugShowNormals())
        {
            if (debugShader)
            {
                debugShader->use();
                // Pasar matrices a debug shader
                glUniformMatrix4fv(glGetUniformLocation(debugShader->ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
                glUniformMatrix4fv(glGetUniformLocation(debugShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
                glUniformMatrix4fv(glGetUniformLocation(debugShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

                // Color para normales
                glUniform3f(glGetUniformLocation(debugShader->ID, "color"), 1.0f, 0.0f, 0.0f);

                // Llamar a DrawNormals del mesh
                mesh->DrawNormals(modelMatrix, 0.05f);
            }
        }
    }

    // Dibujar hijos recursivamente
    for (GameObject* child : go->GetChildren())
    {
        DrawGameObjects(child);
    }
}

void OpenGL::LoadGeometry(const std::string& type) {
    // Limpiar geometría anterior
    if (currentGeometry) {
        currentGeometry->Cleanup();
        delete currentGeometry;
        currentGeometry = nullptr;
    }

    // Limpiar modelo FBX si existe
    if (fbxModel) {
        delete fbxModel;
        fbxModel = nullptr;
    }

    // Limpiar escena de GameObjects
    Application::GetInstance().moduleScene->ClearScene();
    Application::GetInstance().moduleScene->Start(); // Re-crear el root

    // Crear nueva geometría
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

    // Usar textura checkerboard
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

    // Debug shader for drawing normals (simple colored lines)
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

    // Load default texture
    texture = Texture::LoadTexture("../Assets/Textures/wall.jpg");
    if (texture == 0)
    {
        std::cout << "Default texture not found, generating checkerboard..." << std::endl;
        texture = Texture::CreateCheckerboardTexture(512, 512, 32);
    }

    // Crear el grid PRIMERO
    CreateGrid(20);

    // ? Load default model usando ModuleScene y escalarlo
    try
    {
        std::cout << "Loading BakerHouse.fbx..." << std::endl;

        Application::GetInstance().moduleScene->LoadModel("../Assets/Models/BakerHouse.fbx");

        // Escalar y rotar el modelo inicial
        GameObject* root = Application::GetInstance().moduleScene->GetRoot();
        if (root && root->GetChildren().size() > 0)
        {
            GameObject* bakerHouse = root->GetChildren().back();
            std::cout << "BakerHouse GameObject name: " << bakerHouse->GetName() << std::endl;

            ComponentTransform* transform = bakerHouse->GetComponent<ComponentTransform>();
            if (transform)
            {
                // Escala más pequeña
                transform->SetScale(glm::vec3(0.01f)); // Ajusta según necesites

                // Rotar 90 grados en el eje X para ponerlo de pie
                // Usando glm::angleAxis que está en glm/gtc/quaternion.hpp
                glm::quat rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                transform->SetRotation(rotation);
            }
            else
            {
                std::cout << "BakerHouse has no transform component!" << std::endl;
            }

            // Verificar que tenga mesh
            ComponentMesh* mesh = bakerHouse->GetComponent<ComponentMesh>();
            if (mesh)
            {
                std::cout << "BakerHouse has mesh component" << std::endl;
            }
            else
            {
                std::cout << "BakerHouse has NO mesh component!" << std::endl;
            }
        }
        else
        {
            std::cout << "ERROR: Root has no children after loading model!" << std::endl;
        }

        std::cout << "Successfully loaded FBX model via ModuleScene" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "ERROR: Could not load FBX model - " << e.what() << std::endl;
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
                // Limpiar geometría procedural
                if (currentGeometry) {
                    currentGeometry->Cleanup();
                    delete currentGeometry;
                    currentGeometry = nullptr;
                    isGeometryActive = false;
                }

                // Limpiar modelo FBX antiguo
                if (fbxModel) {
                    delete fbxModel;
                    fbxModel = nullptr;
                }

                // Cargar modelo usando ModuleScene (crea GameObjects automáticamente)
                app.moduleScene->LoadModel(filePath.c_str());

                // Escalar y rotar el modelo recién cargado
                GameObject* root = app.moduleScene->GetRoot();
                if (root && root->GetChildren().size() > 0)
                {
                    // El modelo dropeado suele ser el último hijo del root
                    GameObject* droppedModel = root->GetChildren().back();
                    ComponentTransform* transform = droppedModel->GetComponent<ComponentTransform>();

                    if (transform)
                    {
                        // Misma escala y rotación que al inicio
                        transform->SetScale(glm::vec3(0.01f)); // Ajusta según necesites

                        // Rotar para que esté de pie
                        glm::quat rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                        transform->SetRotation(rotation);
                    }
                }

                std::cout << "Loaded 3D model via ModuleScene: " << filePath << std::endl;
            }
            else if (ext == "jpg" || ext == "png" || ext == "tga" || ext == "bmp" || ext == "dds")
            {
                GLuint newTex = (ext == "dds") ?
                    Texture::LoadDDSTexture(filePath.c_str()) :
                    Texture::LoadTexture(filePath.c_str());

                if (newTex)
                {
                    // Solo actualizar la referencia, no eliminar (los componentes manejan sus texturas)
                    texture = newTex;

                    // Aplicar textura a todos los GameObjects de la escena
                    GameObject* root = app.moduleScene->GetRoot();
                    if (root)
                    {
                        ApplyTextureToGameObjects(root, newTex, filePath.c_str());
                    }

                    std::cout << "Applied new texture to scene: " << filePath << std::endl;
                }
            }
            else
            {
                std::cout << "Unsupported file format: " << filePath << std::endl;
            }
        }

        app.input->droppedFiles.clear();
    }

    // Dibujar grid
    DrawGrid();

    if (!shader) return true;

    // Renderizar geometría procedural o GameObjects de la escena
    if (isGeometryActive && currentGeometry)
    {
        // Renderizado de geometría procedural (antigua forma)
        shader->use();

        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::rotate(modelMatrix, rotationAngle, glm::vec3(0, 1, 0));

        view = app.camera->getViewMatrix();
        projection = app.camera->getProjectionMatrix();

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
        // Renderizar GameObjects de la escena (nueva forma)
        GameObject* root = app.moduleScene->GetRoot();
        if (root)
        {
            DrawGameObjects(root);
        }
    }

    return true;
}

void OpenGL::ApplyTextureToGameObjects(GameObject* go, GLuint texID, const char* path)
{
    if (!go)
        return;

    // Aplicar textura al material de este GameObject
    ComponentMaterial* material = go->GetComponent<ComponentMaterial>();
    if (material)
    {
        material->SetTexture(texID, path);
    }

    // Aplicar recursivamente a los hijos
    for (GameObject* child : go->GetChildren())
    {
        ApplyTextureToGameObjects(child, texID, path);
    }
}

bool OpenGL::CleanUp()
{
    std::cout << "Cleaning up OpenGL resources..." << std::endl;

    // PRIMERO: Limpiar la escena (esto eliminará los GameObjects y sus componentes)
    Application::GetInstance().moduleScene->CleanUp();

    // Delete geometry resources
    if (currentGeometry) {
        currentGeometry->Cleanup();
        delete currentGeometry;
        currentGeometry = nullptr;
    }

    // Delete grid resources
    if (gridVBO) {
        glDeleteBuffers(1, &gridVBO);
        gridVBO = 0;
    }
    if (gridVAO) {
        glDeleteVertexArrays(1, &gridVAO);
        gridVAO = 0;
    }

    // Solo eliminar la textura si es válida y no está siendo usada por componentes
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

    if (glContext)
    {
        SDL_GL_DestroyContext(static_cast<SDL_GLContext>(glContext));
        glContext = nullptr;
    }

    std::cout << "OpenGL cleanup complete" << std::endl;
    return true;
}