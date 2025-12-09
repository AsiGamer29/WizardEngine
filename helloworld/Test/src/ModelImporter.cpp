#include "MeshImporter.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <filesystem>

namespace WizardEngine {

    WizardMeshData MeshImporter::Import(const aiMesh* mesh) {
        WizardMeshData meshData;

        if (!mesh) {
            std::cerr << "[MeshImporter] Null mesh provided" << std::endl;
            return meshData;
        }

        // Import vertices
        meshData.vertices.reserve(mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            WizardVertex vertex;

            // Position
            vertex.position.x = mesh->mVertices[i].x;
            vertex.position.y = mesh->mVertices[i].y;
            vertex.position.z = mesh->mVertices[i].z;

            // Normal
            if (mesh->HasNormals()) {
                vertex.normal.x = mesh->mNormals[i].x;
                vertex.normal.y = mesh->mNormals[i].y;
                vertex.normal.z = mesh->mNormals[i].z;
            }
            else {
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            // Texture coordinates
            if (mesh->mTextureCoords[0]) {
                vertex.texCoords.x = mesh->mTextureCoords[0][i].x;
                vertex.texCoords.y = mesh->mTextureCoords[0][i].y;
            }
            else {
                vertex.texCoords = glm::vec2(0.0f, 0.0f);
            }

            meshData.vertices.push_back(vertex);
        }

        // Import indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                meshData.indices.push_back(face.mIndices[j]);
            }
        }

        // Calculate AABB
        CalculateAABB(meshData);

        std::cout << "[MeshImporter] Imported mesh: "
            << meshData.vertices.size() << " vertices, "
            << meshData.indices.size() << " indices" << std::endl;

        return meshData;
    }

    bool MeshImporter::Save(const WizardMeshData& meshData, const std::string& filepath) {
        std::filesystem::path path(filepath);
        std::filesystem::create_directories(path.parent_path());

        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[MeshImporter] Failed to save: " << filepath << std::endl;
            return false;
        }

        // Write header
        WizardMeshHeader header;
        std::memcpy(header.magic, "WZM", 4);
        header.version = 1;
        header.vertexCount = static_cast<unsigned int>(meshData.vertices.size());
        header.indexCount = static_cast<unsigned int>(meshData.indices.size());
        header.aabbMin = meshData.aabbMin;
        header.aabbMax = meshData.aabbMax;

        file.write(reinterpret_cast<const char*>(&header), sizeof(WizardMeshHeader));

        // Write vertex data
        file.write(reinterpret_cast<const char*>(meshData.vertices.data()),
            meshData.vertices.size() * sizeof(WizardVertex));

        // Write index data
        file.write(reinterpret_cast<const char*>(meshData.indices.data()),
            meshData.indices.size() * sizeof(unsigned int));

        file.close();

        size_t fileSize = GetFileSize(filepath);
        std::cout << "[MeshImporter] Saved: " << filepath
            << " (" << fileSize << " bytes)" << std::endl;

        return true;
    }

    bool MeshImporter::Load(const std::string& filepath, WizardMeshData& outMeshData) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[MeshImporter] Failed to load: " << filepath << std::endl;
            return false;
        }

        // Read header
        WizardMeshHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(WizardMeshHeader));

        // Verify magic number
        if (std::strncmp(header.magic, "WZM", 3) != 0) {
            std::cerr << "[MeshImporter] Invalid WZM file: " << filepath << std::endl;
            file.close();
            return false;
        }

        // Verify version
        if (header.version != 1) {
            std::cerr << "[MeshImporter] Unsupported version: " << header.version << std::endl;
            file.close();
            return false;
        }

        // Read vertex data
        outMeshData.vertices.resize(header.vertexCount);
        file.read(reinterpret_cast<char*>(outMeshData.vertices.data()),
            header.vertexCount * sizeof(WizardVertex));

        // Read index data
        outMeshData.indices.resize(header.indexCount);
        file.read(reinterpret_cast<char*>(outMeshData.indices.data()),
            header.indexCount * sizeof(unsigned int));

        // Read AABB
        outMeshData.aabbMin = header.aabbMin;
        outMeshData.aabbMax = header.aabbMax;

        file.close();

        std::cout << "[MeshImporter] Loaded: " << filepath
            << " (" << header.vertexCount << " vertices, "
            << header.indexCount << " indices)" << std::endl;

        return true;
    }

    size_t MeshImporter::GetFileSize(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return 0;
        size_t size = file.tellg();
        file.close();
        return size;
    }

    void MeshImporter::CalculateAABB(WizardMeshData& meshData) {
        if (meshData.vertices.empty()) {
            meshData.aabbMin = glm::vec3(0.0f);
            meshData.aabbMax = glm::vec3(0.0f);
            return;
        }

        meshData.aabbMin = meshData.vertices[0].position;
        meshData.aabbMax = meshData.vertices[0].position;

        for (const auto& vertex : meshData.vertices) {
            meshData.aabbMin = glm::min(meshData.aabbMin, vertex.position);
            meshData.aabbMax = glm::max(meshData.aabbMax, vertex.position);
        }
    }

} // namespace WizardEngine