#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <IL/il.h>
#include <IL/ilu.h>

#include "Mesh.h"
#include "Shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

using namespace std;

// Function declaration for loading textures using DevIL
unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);

class Model
{
public:
    // Model data
    vector<MeshTexture> textures_loaded;
    vector<Mesh> meshes;
    string directory;
    bool gammaCorrection;

    // Constructor – loads a model right away
    Model(string const& path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    // Draws the entire model (all its meshes)
    void Draw(Shader& shader)
    {
        for (size_t i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

    // Frees GPU textures for all meshes
    void ClearTextures()
    {
        for (auto& mesh : meshes)
        {
            for (auto& tex : mesh.textures)
            {
                glDeleteTextures(1, &tex.id);
            }
            mesh.textures.clear();
        }
    }

private:
    // Loads a model from file using Assimp
    void loadModel(string const& path)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            cout << "[ERROR] Failed to load model: " << importer.GetErrorString() << endl;
            return;
        }

        directory = path.substr(0, path.find_last_of('/'));

        cout << "[MODEL] Successfully loaded: " << path << endl;
        cout << "         Mesh count: " << scene->mNumMeshes << endl;

        processNode(scene->mRootNode, scene);
    }

    // Recursively processes all nodes in the Assimp scene
    void processNode(aiNode* node, const aiScene* scene)
    {
        // Process all meshes in this node
        for (size_t i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }

        // Then process all child nodes
        for (size_t i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    // Converts an Assimp mesh into our own Mesh class
    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<MeshTexture> textures;

        // Extract vertex data
        for (size_t i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;

            // Positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            // Normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            else
            {
                vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            // Texture coordinates, tangents, and bitangents
            if (mesh->mTextureCoords[0])
            {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;

                if (mesh->HasTangentsAndBitangents())
                {
                    vector.x = mesh->mTangents[i].x;
                    vector.y = mesh->mTangents[i].y;
                    vector.z = mesh->mTangents[i].z;
                    vertex.Tangent = vector;

                    vector.x = mesh->mBitangents[i].x;
                    vector.y = mesh->mBitangents[i].y;
                    vector.z = mesh->mBitangents[i].z;
                    vertex.Bitangent = vector;
                }
                else
                {
                    vertex.Tangent = glm::vec3(0.0f);
                    vertex.Bitangent = glm::vec3(0.0f);
                }
            }
            else
            {
                vertex.TexCoords = glm::vec2(0.0f);
                vertex.Tangent = glm::vec3(0.0f);
                vertex.Bitangent = glm::vec3(0.0f);
            }

            // Initialize bone data to default values
            for (int j = 0; j < MAX_BONE_INFLUENCE; j++)
            {
                vertex.m_BoneIDs[j] = -1;
                vertex.m_Weights[j] = 0.0f;
            }

            vertices.push_back(vertex);
        }

        // Process indices
        for (size_t i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (size_t j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // Process material textures
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        vector<MeshTexture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        vector<MeshTexture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        vector<MeshTexture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        vector<MeshTexture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        cout << "[MESH] Processed: " << vertices.size() << " vertices, "
            << indices.size() << " indices, " << textures.size() << " textures" << endl;

        return Mesh(vertices, indices, textures);
    }

    // Loads all textures of a specific type from a material
    vector<MeshTexture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
    {
        vector<MeshTexture> textures;
        for (size_t i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);

            bool skip = false;
            for (size_t j = 0; j < textures_loaded.size(); j++)
            {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }

            if (!skip)
            {
                MeshTexture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }
};

// Inline texture loading function using DevIL
inline unsigned int TextureFromFile(const char* path, const string& directory, bool gamma)
{
    string filename = string(path);
    vector<string> pathsToTry;

    pathsToTry.push_back(directory + '/' + filename);

    size_t lastSlash = filename.find_last_of("/\\");
    string filenameOnly = (lastSlash != string::npos) ? filename.substr(lastSlash + 1) : filename;
    pathsToTry.push_back(directory + '/' + filenameOnly);
    pathsToTry.push_back(directory + "/../Textures/" + filenameOnly);
    pathsToTry.push_back(directory + "/Textures/" + filenameOnly);
    pathsToTry.push_back("../Assets/Textures/" + filenameOnly);

    ILuint imgID;
    ilGenImages(1, &imgID);
    ilBindImage(imgID);

    bool loaded = false;
    string loadedPath;

    // Try multiple fallback paths for texture
    for (const auto& tryPath : pathsToTry)
    {
        cout << "[TEXTURE] Trying: " << tryPath << endl;
        if (ilLoadImage(tryPath.c_str()))
        {
            loaded = true;
            loadedPath = tryPath;
            cout << "  -> Found and loaded successfully!" << endl;
            break;
        }
    }

    // If texture failed to load, create a placeholder (magenta)
    if (!loaded)
    {
        ILenum error = ilGetError();
        cout << "[ERROR] Texture could not be loaded from any location." << endl;
        for (auto& p : pathsToTry) cout << "  - " << p << endl;
        cout << "  DevIL Error: " << error << " -> " << iluErrorString(error) << endl;
        ilDeleteImages(1, &imgID);

        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        unsigned char magentaPixel[] = { 255, 0, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, magentaPixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        return textureID;
    }

    // Convert to RGBA
    if (!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE))
    {
        ILenum error = ilGetError();
        cout << "[ERROR] Failed to convert image to RGBA: " << iluErrorString(error) << endl;
        ilDeleteImages(1, &imgID);

        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        unsigned char magentaPixel[] = { 255, 0, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, magentaPixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        return textureID;
    }

    // Upload texture to OpenGL
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    int width = ilGetInteger(IL_IMAGE_WIDTH);
    int height = ilGetInteger(IL_IMAGE_HEIGHT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    ilDeleteImages(1, &imgID);

    cout << "[TEXTURE] Loaded: " << loadedPath << " (" << width << "x" << height << ")" << endl;

    return textureID;
}

#endif
