#pragma once
#include "Module.h"
#include "Resource.h"
#include "MetaFile.h"
#include <map>
#include <string>
#include <vector>
#include <filesystem>

namespace WizardEngine {

    class ModuleResources : public Module {
    public:
        ModuleResources();
        ~ModuleResources() override;

        bool Start() override;
        bool Update() override;
        bool CleanUp() override;

        // Resource management
        uint64_t Find(const std::string& fileInAssets) const;
        uint64_t ImportFile(const std::string& newFileInAssets);
        uint64_t GenerateNewUID();

        // Resource requests (with reference counting)
        Resource* RequestResource(uint64_t uid);
        const Resource* RequestResource(uint64_t uid) const;
        void ReleaseResource(uint64_t uid);

        // Get all resources info for UI
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

    private:
        Resource* CreateNewResource(const std::string& assetsFile, ResourceType type);
        ResourceType GetResourceTypeFromExtension(const std::string& ext) const;

        bool ProcessFile(const std::string& filepath);
        void CheckForChanges(); // Called every few seconds

        std::map<uint64_t, Resource*> resources;
        std::map<std::string, uint64_t> pathToUID; // Fast lookup

        float timeSinceLastCheck;
        const float checkInterval = 2.0f; // Check for file changes every 2 seconds
    };

} // namespace WizardEngine