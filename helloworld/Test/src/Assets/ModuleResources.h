#pragma once
#include "Module.h"
#include "Resource.h"
#include <map>
#include <string>
#include <vector>
#include <cstdint>

namespace WizardEngine {

    class AssetManager; // Forward declaration

    class ModuleResources : public Module {
    public:
        ModuleResources();
        ~ModuleResources() override;

        bool Start() override;
        bool Update() override;
        bool CleanUp() override;

        // Find resource UID by asset path
        uint64_t Find(const std::string& fileInAssets) const;

        // Import new file and return UID
        uint64_t ImportFile(const std::string& newFileInAssets);

        // Generate unique UID
        uint64_t GenerateNewUID();

        // Resource requests with reference counting
        Resource* RequestResource(uint64_t uid);
        const Resource* RequestResource(uint64_t uid) const;
        void ReleaseResource(uint64_t uid);

        // Get resource info for UI
        struct ResourceInfo {
            uint64_t uid;
            std::string name;
            std::string assetPath;
            std::string libraryPath;
            ResourceType type;
            bool inMemory;
            uint32_t references;
        };

        std::vector<ResourceInfo> GetAllResourcesInfo() const;
        ResourceInfo GetResourceInfo(uint64_t uid) const;

        // Folder scanning
        void ScanAssetsFolder();

        // Get AssetManager instance
        AssetManager* GetAssetManager() { return assetManager; }

    private:
        // Create new resource entry
        Resource* CreateNewResource(const std::string& assetsFile, ResourceType type);

        // Determine type from extension
        ResourceType GetResourceTypeFromExtension(const std::string& ext) const;

        // Process individual file
        bool ProcessFile(const std::string& filepath);

        // Check for file changes periodically
        void CheckForChanges();

        // Resource database
        std::map<uint64_t, Resource*> resources;
        std::map<std::string, uint64_t> pathToUID; // Fast lookup by path

        // Asset manager (owned by this module)
        AssetManager* assetManager;

        // File monitoring
        float timeSinceLastCheck;
        const float checkInterval = 2.0f; // Check every 2 seconds
    };

} // namespace WizardEngine