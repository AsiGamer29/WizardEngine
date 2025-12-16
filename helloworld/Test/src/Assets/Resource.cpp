#include "Resource.h"
#include "TextureImporter.h"
#include "MeshImporter.h"
#include <glad/glad.h>
#include <iostream>

namespace WizardEngine {

    // Base Resource
    Resource::Resource(uint64_t uid, ResourceType type)
        : uid(uid), type(type), loadedToMemory(false), referenceCount(0) {
    }

    Resource::~Resource() {
    }

    // ResourceTexture
    ResourceTexture::ResourceTexture(uint64_t uid)
        : Resource(uid, ResourceType::TEXTURE), gpuId(0), width(0), height(0), channels(0) {
    }

    ResourceTexture::~ResourceTexture() {
        UnloadFromMemory();
    }

    bool ResourceTexture::LoadToMemory() {
        if (loadedToMemory) return true;

        WizardTextureData texData;
        if (!TextureImporter::Load(libraryFile, texData)) {
            std::cerr << "[ResourceTexture] Failed to load: " << libraryFile << std::endl;
            return false;
        }

        glGenTextures(1, &gpuId);
        glBindTexture(GL_TEXTURE_2D, gpuId);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
            texData.width, texData.height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, texData.data.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        width = texData.width;
        height = texData.height;
        channels = texData.channels;
        loadedToMemory = true;

        std::cout << "[ResourceTexture] Loaded to GPU: " << assetFile << std::endl;
        return true;
    }

    void ResourceTexture::UnloadFromMemory() {
        if (gpuId != 0) {
            glDeleteTextures(1, &gpuId);
            gpuId = 0;
        }
        loadedToMemory = false;
    }

    // ResourceMesh
    ResourceMesh::ResourceMesh(uint64_t uid)
        : Resource(uid, ResourceType::MESH), vao(0), vbo(0), ebo(0), vertexCount(0), indexCount(0) {
    }

    ResourceMesh::~ResourceMesh() {
        UnloadFromMemory();
    }

    bool ResourceMesh::LoadToMemory() {
        if (loadedToMemory) return true;

        WizardMeshData meshData;
        if (!MeshImporter::Load(libraryFile, meshData)) {
            std::cerr << "[ResourceMesh] Failed to load: " << libraryFile << std::endl;
            return false;
        }

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, meshData.vertices.size() * sizeof(WizardVertex),
            meshData.vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshData.indices.size() * sizeof(unsigned int),
            meshData.indices.data(), GL_STATIC_DRAW);

        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(WizardVertex), (void*)0);

        // Normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(WizardVertex), (void*)offsetof(WizardVertex, normal));

        // TexCoords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(WizardVertex), (void*)offsetof(WizardVertex, texCoords));

        glBindVertexArray(0);

        vertexCount = meshData.vertices.size();
        indexCount = meshData.indices.size();
        loadedToMemory = true;

        std::cout << "[ResourceMesh] Loaded to GPU: " << assetFile << std::endl;
        return true;
    }

    void ResourceMesh::UnloadFromMemory() {
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
        if (ebo != 0) glDeleteBuffers(1, &ebo);
        vao = vbo = ebo = 0;
        loadedToMemory = false;
    }

    // ResourceModel
    ResourceModel::ResourceModel(uint64_t uid)
        : Resource(uid, ResourceType::MODEL) {
    }

    ResourceModel::~ResourceModel() {
        UnloadFromMemory();
    }

    bool ResourceModel::LoadToMemory() {
        // Models don't load directly, they reference other resources
        loadedToMemory = true;
        return true;
    }

    void ResourceModel::UnloadFromMemory() {
        loadedToMemory = false;
    }

} // namespace WizardEngine