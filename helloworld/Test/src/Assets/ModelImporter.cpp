#include "ModelImporter.h"
#include "MeshImporter.h"
#include "TextureImporter.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/type_ptr.hpp>

namespace WizardEngine {

    ModelImporter::ImportTiming ModelImporter::lastTiming = {};

    WizardModelData ModelImporter::Import(const std::string& filepath,
        const std::string& outputDirectory) {
        WizardModelData modelData;

        // Reset timing
        lastTiming = {};

        auto startFBX = std::chrono::high_resolution_clock::now();

        // Load FBX using Assimp
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(filepath,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "[ModelImporter] Assimp Error: " << importer.GetErrorString() << std::endl;
            return modelData;
        }

        auto endFBX = std::chrono::high_resolution_clock::now();
        lastTiming.fbxLoadTime = std::chrono::duration<double>(endFBX - startFBX).count();

        // Get FBX file size
        std::ifstream fbxFile(filepath, std::ios::binary | std::ios::ate);
        if (fbxFile.is_open()) {
            lastTiming.fbxFileSize = fbxFile.tellg();
            fbxFile.close();
        }

        std::cout << "[ModelImporter] Loaded FBX: " << filepath << std::endl;
        std::cout << "[ModelImporter] Meshes: " << scene->mNumMeshes << std::endl;
        std::cout << "[ModelImporter] Materials: " << scene->mNumMaterials << std::endl;
        std::cout << "[ModelImporter] FBX Load Time: " << lastTiming.fbxLoadTime << "s" << std::endl;

        auto startCustomSave = std::chrono::high_resolution_clock::now();

        // Create output directories
        std::filesystem::create_directories(outputDirectory + "/Meshes");
        std::filesystem::create_directories(outputDirectory + "/Materials");
        std::filesystem::create_directories(outputDirectory + "/Textures");

        // Import meshes
        for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
            WizardMeshData meshData = MeshImporter::Import(scene->mMeshes[i]);

            std::string meshFilename = "mesh_" + std::to_string(i) + ".wzm";
            std::string meshPath = outputDirectory + "/Meshes/" + meshFilename;

            MeshImporter::Save(meshData, meshPath);

            WizardMeshReference meshRef;
            meshRef.meshFilepath = meshPath;
            meshRef.materialIndex = scene->mMeshes[i]->mMaterialIndex;
            modelData.meshes.push_back(meshRef);
        }

        // Import materials
        std::string baseDirectory = filepath.substr(0, filepath.find_last_of("/\\"));

        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial* material = scene->mMaterials[i];
            WizardMaterialData matData;

            aiString name;
            material->Get(AI_MATKEY_NAME, name);
            matData.name = name.C_Str();

            aiColor3D color;
            material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            matData.diffuseColor = glm::vec3(color.r, color.g, color.b);

            material->Get(AI_MATKEY_COLOR_SPECULAR, color);
            matData.specularColor = glm::vec3(color.r, color.g, color.b);

            float shininess = 32.0f;
            material->Get(AI_MATKEY_SHININESS, shininess);
            matData.shininess = shininess;

            // Import diffuse texture
            if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
                aiString texPath;
                material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);

                // Try multiple paths for texture
                std::vector<std::string> possiblePaths = {
                    baseDirectory + "/" + texPath.C_Str(),
                    baseDirectory + "/../Textures/" + ExtractFilename(texPath.C_Str()),
                    baseDirectory + "/Textures/" + ExtractFilename(texPath.C_Str()),
                    "Assets/Textures/" + ExtractFilename(texPath.C_Str())
                };

                bool textureLoaded = false;
                for (const auto& tryPath : possiblePaths) {
                    if (std::filesystem::exists(tryPath)) {
                        WizardTextureData texData = TextureImporter::Import(tryPath);
                        if (!texData.data.empty()) {
                            std::string texFilename = ExtractFilename(texPath.C_Str());
                            size_t dotPos = texFilename.find_last_of('.');
                            if (dotPos != std::string::npos) {
                                texFilename = texFilename.substr(0, dotPos);
                            }
                            texFilename += ".wzt";

                            std::string texSavePath = outputDirectory + "/Textures/" + texFilename;

                            TextureImporter::Save(texData, texSavePath);
                            matData.diffuseTexture = texSavePath;
                            textureLoaded = true;
                            break;
                        }
                    }
                }

                if (!textureLoaded) {
                    std::cout << "[ModelImporter] Warning: Could not find texture: "
                        << texPath.C_Str() << std::endl;
                }
            }

            modelData.materials.push_back(matData);
        }

        // Import node hierarchy
        modelData.rootNodeIndex = 0;
        ProcessNode(scene, scene->mRootNode, modelData, -1, outputDirectory);

        auto endCustomSave = std::chrono::high_resolution_clock::now();
        lastTiming.customSaveTime = std::chrono::duration<double>(endCustomSave - startCustomSave).count();

        std::cout << "[ModelImporter] Custom Save Time: " << lastTiming.customSaveTime << "s" << std::endl;

        return modelData;
    }

    void ModelImporter::ProcessNode(const aiScene* scene, aiNode* node,
        WizardModelData& modelData, int parentIndex,
        const std::string& outputDirectory) {
        WizardNodeData nodeData;
        nodeData.name = node->mName.C_Str();
        nodeData.parentIndex = parentIndex;

        // Copy transformation matrix
        aiMatrix4x4 transform = node->mTransformation;
        nodeData.transformation = glm::mat4(
            transform.a1, transform.b1, transform.c1, transform.d1,
            transform.a2, transform.b2, transform.c2, transform.d2,
            transform.a3, transform.b3, transform.c3, transform.d3,
            transform.a4, transform.b4, transform.c4, transform.d4
        );

        // Copy mesh indices
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            nodeData.meshIndices.push_back(node->mMeshes[i]);
        }

        int currentIndex = static_cast<int>(modelData.nodes.size());
        modelData.nodes.push_back(nodeData);

        // Process children
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            int childIndex = static_cast<int>(modelData.nodes.size());
            modelData.nodes[currentIndex].childIndices.push_back(childIndex);
            ProcessNode(scene, node->mChildren[i], modelData, currentIndex, outputDirectory);
        }
    }

    bool ModelImporter::Save(const WizardModelData& modelData, const std::string& filepath) {
        std::filesystem::path path(filepath);
        std::filesystem::create_directories(path.parent_path());

        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[ModelImporter] Failed to save: " << filepath << std::endl;
            return false;
        }

        // Write header
        WizardModelHeader header;
        std::memcpy(header.magic, "WZD", 4);
        header.version = 1;
        header.meshCount = static_cast<unsigned int>(modelData.meshes.size());
        header.materialCount = static_cast<unsigned int>(modelData.materials.size());
        header.nodeCount = static_cast<unsigned int>(modelData.nodes.size());
        header.rootNodeIndex = modelData.rootNodeIndex;

        file.write(reinterpret_cast<const char*>(&header), sizeof(WizardModelHeader));

        // Write mesh references
        for (const auto& mesh : modelData.meshes) {
            unsigned int pathLength = static_cast<unsigned int>(mesh.meshFilepath.size());
            file.write(reinterpret_cast<const char*>(&pathLength), sizeof(unsigned int));
            file.write(mesh.meshFilepath.c_str(), pathLength);
            file.write(reinterpret_cast<const char*>(&mesh.materialIndex), sizeof(unsigned int));
        }

        // Write materials
        for (const auto& mat : modelData.materials) {
            unsigned int nameLength = static_cast<unsigned int>(mat.name.size());
            file.write(reinterpret_cast<const char*>(&nameLength), sizeof(unsigned int));
            file.write(mat.name.c_str(), nameLength);

            unsigned int texLength = static_cast<unsigned int>(mat.diffuseTexture.size());
            file.write(reinterpret_cast<const char*>(&texLength), sizeof(unsigned int));
            file.write(mat.diffuseTexture.c_str(), texLength);

            file.write(reinterpret_cast<const char*>(&mat.diffuseColor), sizeof(glm::vec3));
            file.write(reinterpret_cast<const char*>(&mat.specularColor), sizeof(glm::vec3));
            file.write(reinterpret_cast<const char*>(&mat.shininess), sizeof(float));
        }

        // Write nodes
        for (const auto& node : modelData.nodes) {
            unsigned int nameLength = static_cast<unsigned int>(node.name.size());
            file.write(reinterpret_cast<const char*>(&nameLength), sizeof(unsigned int));
            file.write(node.name.c_str(), nameLength);

            file.write(reinterpret_cast<const char*>(&node.transformation), sizeof(glm::mat4));
            file.write(reinterpret_cast<const char*>(&node.parentIndex), sizeof(int));

            unsigned int childCount = static_cast<unsigned int>(node.childIndices.size());
            file.write(reinterpret_cast<const char*>(&childCount), sizeof(unsigned int));
            file.write(reinterpret_cast<const char*>(node.childIndices.data()),
                childCount * sizeof(int));

            unsigned int meshCount = static_cast<unsigned int>(node.meshIndices.size());
            file.write(reinterpret_cast<const char*>(&meshCount), sizeof(unsigned int));
            file.write(reinterpret_cast<const char*>(node.meshIndices.data()),
                meshCount * sizeof(int));
        }

        file.close();

        std::cout << "[ModelImporter] Saved WZD: " << filepath << std::endl;
        return true;
    }

    bool ModelImporter::Load(const std::string& filepath, WizardModelData& outModelData) {
        auto startLoad = std::chrono::high_resolution_clock::now();

        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[ModelImporter] Failed to load: " << filepath << std::endl;
            return false;
        }

        // Read header
        WizardModelHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(WizardModelHeader));

        if (std::strncmp(header.magic, "WZD", 3) != 0) {
            std::cerr << "[ModelImporter] Invalid WZD file: " << filepath << std::endl;
            file.close();
            return false;
        }

        if (header.version != 1) {
            std::cerr << "[ModelImporter] Unsupported version: " << header.version << std::endl;
            file.close();
            return false;
        }

        outModelData.meshes.clear();
        outModelData.materials.clear();
        outModelData.nodes.clear();
        outModelData.rootNodeIndex = header.rootNodeIndex;

        // Read mesh references
        for (unsigned int i = 0; i < header.meshCount; i++) {
            WizardMeshReference meshRef;

            unsigned int pathLength;
            file.read(reinterpret_cast<char*>(&pathLength), sizeof(unsigned int));
            meshRef.meshFilepath.resize(pathLength);
            file.read(&meshRef.meshFilepath[0], pathLength);

            file.read(reinterpret_cast<char*>(&meshRef.materialIndex), sizeof(unsigned int));

            outModelData.meshes.push_back(meshRef);
        }

        // Read materials
        for (unsigned int i = 0; i < header.materialCount; i++) {
            WizardMaterialData mat;

            unsigned int nameLength;
            file.read(reinterpret_cast<char*>(&nameLength), sizeof(unsigned int));
            mat.name.resize(nameLength);
            file.read(&mat.name[0], nameLength);

            unsigned int texLength;
            file.read(reinterpret_cast<char*>(&texLength), sizeof(unsigned int));
            mat.diffuseTexture.resize(texLength);
            file.read(&mat.diffuseTexture[0], texLength);

            file.read(reinterpret_cast<char*>(&mat.diffuseColor), sizeof(glm::vec3));
            file.read(reinterpret_cast<char*>(&mat.specularColor), sizeof(glm::vec3));
            file.read(reinterpret_cast<char*>(&mat.shininess), sizeof(float));

            outModelData.materials.push_back(mat);
        }

        // Read nodes
        for (unsigned int i = 0; i < header.nodeCount; i++) {
            WizardNodeData node;

            unsigned int nameLength;
            file.read(reinterpret_cast<char*>(&nameLength), sizeof(unsigned int));
            node.name.resize(nameLength);
            file.read(&node.name[0], nameLength);

            file.read(reinterpret_cast<char*>(&node.transformation), sizeof(glm::mat4));
            file.read(reinterpret_cast<char*>(&node.parentIndex), sizeof(int));

            unsigned int childCount;
            file.read(reinterpret_cast<char*>(&childCount), sizeof(unsigned int));
            node.childIndices.resize(childCount);
            file.read(reinterpret_cast<char*>(node.childIndices.data()),
                childCount * sizeof(int));

            unsigned int meshCount;
            file.read(reinterpret_cast<char*>(&meshCount), sizeof(unsigned int));
            node.meshIndices.resize(meshCount);
            file.read(reinterpret_cast<char*>(node.meshIndices.data()),
                meshCount * sizeof(int));

            outModelData.nodes.push_back(node);
        }

        file.close();

        auto endLoad = std::chrono::high_resolution_clock::now();
        lastTiming.customLoadTime = std::chrono::duration<double>(endLoad - startLoad).count();

        // Get file size
        std::ifstream sizeFile(filepath, std::ios::binary | std::ios::ate);
        if (sizeFile.is_open()) {
            lastTiming.customFileSize = sizeFile.tellg();
            sizeFile.close();
        }

        std::cout << "[ModelImporter] Loaded WZD: " << filepath << std::endl;
        std::cout << "[ModelImporter] Custom Load Time: " << lastTiming.customLoadTime << "s" << std::endl;

        return true;
    }

    std::string ModelImporter::ExtractFilename(const std::string& path) {
        size_t pos = path.find_last_of("/\\");
        if (pos != std::string::npos) {
            return path.substr(pos + 1);
        }
        return path;
    }

} // namespace WizardEngine