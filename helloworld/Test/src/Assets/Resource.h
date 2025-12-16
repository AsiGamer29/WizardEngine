#pragma once
#include <string>
#include <cstdint>
#include <vector>

namespace WizardEngine {

    enum class ResourceType {
        UNKNOWN,
        TEXTURE,
        MESH,
        MODEL,
        SCENE,
        MATERIAL,
        AUDIO,
        SCRIPT
    };

    class Resource {
    public:
        Resource(uint64_t uid, ResourceType type);
        virtual ~Resource();

        // Getters
        uint64_t GetUID() const { return uid; }
        ResourceType GetType() const { return type; }
        const std::string& GetAssetFile() const { return assetFile; }
        const std::string& GetLibraryFile() const { return libraryFile; }
        bool IsLoadedToMemory() const { return loadedToMemory; }
        uint32_t GetReferenceCount() const { return referenceCount; }

        // Setters
        void SetAssetFile(const std::string& path) { assetFile = path; }
        void SetLibraryFile(const std::string& path) { libraryFile = path; }

        // Reference counting
        void IncrementReference() { referenceCount++; }
        void DecrementReference() { if (referenceCount > 0) referenceCount--; }

        // Memory management
        virtual bool LoadToMemory() = 0;
        virtual void UnloadFromMemory() = 0;

    protected:
        uint64_t uid;
        ResourceType type;
        std::string assetFile;
        std::string libraryFile;
        bool loadedToMemory;
        uint32_t referenceCount;
    };

    // Specific resource types
    class ResourceTexture : public Resource {
    public:
        ResourceTexture(uint64_t uid);
        ~ResourceTexture() override;

        bool LoadToMemory() override;
        void UnloadFromMemory() override;

        unsigned int GetGPUId() const { return gpuId; }
        int GetWidth() const { return width; }
        int GetHeight() const { return height; }

    private:
        unsigned int gpuId;
        int width;
        int height;
        int channels;
    };

    class ResourceMesh : public Resource {
    public:
        ResourceMesh(uint64_t uid);
        ~ResourceMesh() override;

        bool LoadToMemory() override;
        void UnloadFromMemory() override;

        unsigned int GetVAO() const { return vao; }
        unsigned int GetVertexCount() const { return vertexCount; }
        unsigned int GetIndexCount() const { return indexCount; }

    private:
        unsigned int vao;
        unsigned int vbo;
        unsigned int ebo;
        unsigned int vertexCount;
        unsigned int indexCount;
    };

    class ResourceModel : public Resource {
    public:
        ResourceModel(uint64_t uid);
        ~ResourceModel() override;

        bool LoadToMemory() override;
        void UnloadFromMemory() override;

        const std::vector<uint64_t>& GetMeshUIDs() const { return meshUIDs; }
        const std::vector<uint64_t>& GetMaterialUIDs() const { return materialUIDs; }

    private:
        std::vector<uint64_t> meshUIDs;
        std::vector<uint64_t> materialUIDs;
    };

} // namespace WizardEngine