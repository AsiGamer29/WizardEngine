#include "ModuleResources.h"
#include "AssetManager.h"
#include "Application.h"
#include <iostream>
#include <algorithm>
#include <random>

namespace WizardEngine {

    ModuleResources::ModuleResources() : timeSinceLastCheck(0.0f) {
        name = "ModuleResources";
    }

    ModuleResources::~ModuleResources() {
    }

    bool ModuleResources::Start() {
        std::cout << "[ModuleResources] Initializing..." << std::endl;

        // Create necessary directories
        std::filesystem::create_directories("Assets/");
        std::filesystem::create_directories("Library/");

        // Initial scan
        ScanAssetsFolder();

        std::cout << "[ModuleResources] Initialized with " << resources.size() << " resources" << std::endl;
        return true;
    }

    bool ModuleResources::Update() {
        timeSinceLastCheck += 0.016f; // Approximate delta time

        if (timeSinceLastCheck >= checkInterval) {
            CheckForChanges();
            timeSinceLastCheck = 0.0f;
        }

        return true;
    }

    bool ModuleResources::CleanUp() {
        std::cout << "[ModuleResources] Cleaning up..." << std::endl;

        // Delete all resources
        for (auto& pair : resources) {
            delete pair.second;
        }
        resources.clear();
        pathToUID.clear();

        return true;
    }

    uint64_t ModuleResources::Find(const std::string& fileInAssets) const {
        auto it = pathToUID.find(fileInAssets);
        if (it != pathToUID.end()) {
            return it->second;
        }
        return 0; // 0 means not found
    }

    uint64_t ModuleResources::ImportFile(const std::string& newFileInAssets) {
        std::cout << "[ModuleResources] Importing: " << newFileInAssets << std::endl;

        // Check if already exists
        uint64_t existingUID = Find(newFileInAssets);
        if (existingUID != 0) {
            std::cout << "[ModuleResources] File already imported with UID: " << existingUID << std::endl;
            return existingUID;
        }

        // Process with AssetManager
        auto& app = Application::GetInstance();
        if (!app.assetManager) {
            std::cerr << "[ModuleResources] AssetManager not available" << std::endl;
            return 0;
        }

        if (!app.assetManager->ProcessAssetFile(newFileInAssets)) {
            std::cerr << "[ModuleResources] Failed to process asset" << std::endl;
            return 0;
        }

        // Get meta data
        AssetMetaData* metaData = app.assetManager->GetMetaData(newFileInAssets);
        if (!metaData) {
            std::cerr << "[ModuleResources] Failed to get meta data" << std::endl;
            return 0;
        }

        // Create resource
        std::string ext = std::filesystem::path(newFileInAssets).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        ResourceType type = GetResourceTypeFromExtension(ext);

        // Generate UID from meta UUID
        uint64_t uid = std::stoull(metaData->uuid, nullptr, 16);

        Resource* resource = CreateNewResource(newFileInAssets, type);
        if (!resource) {
            std::cerr << "[ModuleResources] Failed to create resource" << std::endl;
            return 0;
        }

        resource->SetAssetFile(newFileInAssets);
        resource->SetLibraryFile(metaData->libraryFile);

        resources[uid] = resource;
        pathToUID[newFileInAssets] = uid;

        std::cout << "[ModuleResources] Imported successfully with UID: " << uid << std::endl;
        return uid;
    }

    uint64_t ModuleResources::GenerateNewUID() {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        static std::uniform_int_distribution<uint64_t> dis;

        uint64_t uid;
        do {
            uid = dis(gen);
        } while (uid == 0 || resources.find(uid) != resources.end());

        return uid;
    }

    Resource* ModuleResources::RequestResource(uint64_t uid) {
        auto it = resources.find(uid);
        if (it == resources.end()) {
            return nullptr;
        }

        Resource* resource = it->second;

        // Load to memory if not loaded
        if (!resource->IsLoadedToMemory()) {
            if (!resource->LoadToMemory()) {
                std::cerr << "[ModuleResources] Failed to load resource to memory: " << uid << std::endl;
                return nullptr;
            }
        }

        resource->IncrementReference();
        std::cout << "[ModuleResources] Resource " << uid << " requested (refs: "
            << resource->GetReferenceCount() << ")" << std::endl;

        return resource;
    }

    const Resource* ModuleResources::RequestResource(uint64_t uid) const {
        auto it = resources.find(uid);
        if (it == resources.end()) {
            return nullptr;
        }
        return it->second;
    }

    void ModuleResources::ReleaseResource(uint64_t uid) {
        auto it = resources.find(uid);
        if (it == resources.end()) {
            return;
        }

        Resource* resource = it->second;
        resource->DecrementReference();

        std::cout << "[ModuleResources] Resource " << uid << " released (refs: "
            << resource->GetReferenceCount() << ")" << std::endl;

        // Unload from memory if no references
        if (resource->GetReferenceCount() == 0 && resource->IsLoadedToMemory()) {
            resource->UnloadFromMemory();
            std::cout << "[ModuleResources] Resource " << uid << " unloaded from memory" << std::endl;
        }
    }

    std::vector<ModuleResources::ResourceInfo> ModuleResources::GetAllResourcesInfo() const {
        std::vector<ResourceInfo> infos;

        for (const auto& pair : resources) {
            ResourceInfo info;
            info.uid = pair.first;
            info.assetPath = pair.second->GetAssetFile();
            info.libraryPath = pair.second->GetLibraryFile();
            info.type = pair.second->GetType();
            info.inMemory = pair.second->IsLoadedToMemory();
            info.references = pair.second->GetReferenceCount();

            // Extract name from path
            info.name = std::filesystem::path(info.assetPath).filename().string();

            infos.push_back(info);
        }

        return infos;
    }

    ModuleResources::ResourceInfo ModuleResources::GetResourceInfo(uint64_t uid) const {
        ResourceInfo info;

        auto it = resources.find(uid);
        if (it != resources.end()) {
            info.uid = uid;
            info.assetPath = it->second->GetAssetFile();
            info.libraryPath = it->second->GetLibraryFile();
            info.type = it->second->GetType();
            info.inMemory = it->second->IsLoadedToMemory();
            info.references = it->second->GetReferenceCount();
            info.name = std::filesystem::path(info.assetPath).filename().string();
        }

        return info;
    }

    void ModuleResources::ScanAssetsFolder() {
        std::cout << "[ModuleResources] Scanning Assets folder..." << std::endl;

        int count = 0;
        for (const auto& entry : std::filesystem::recursive_directory_iterator("Assets/")) {
            if (!entry.is_regular_file()) continue;

            std::string filepath = entry.path().string();
            std::string ext = entry.path().extension().string();

            // Skip .meta files
            if (ext == ".meta") continue;

            // Process file
            if (ProcessFile(filepath)) {
                count++;
            }
        }

        std::cout << "[ModuleResources] Scan complete: " << count << " files processed" << std::endl;
    }

    void ModuleResources::CheckForChanges() {
        // Check if any asset files have been modified
        auto& app = Application::GetInstance();
        if (!app.assetManager) return;

        for (const auto& pair : resources) {
            Resource* resource = pair.second;
            std::string assetPath = resource->GetAssetFile();

            if (app.assetManager->NeedsReimport(assetPath)) {
                std::cout << "[ModuleResources] File changed, reimporting: " << assetPath << std::endl;

                // Unload from memory first
                if (resource->IsLoadedToMemory()) {
                    resource->UnloadFromMemory();
                }

                // Reimport
                app.assetManager->ProcessAssetFile(assetPath);

                // Update library path
                AssetMetaData* metaData = app.assetManager->GetMetaData(assetPath);
                if (metaData) {
                    resource->SetLibraryFile(metaData->libraryFile);
                }
            }
        }
    }

    Resource* ModuleResources::CreateNewResource(const std::string& assetsFile, ResourceType type) {
        uint64_t uid = GenerateNewUID();

        Resource* resource = nullptr;

        switch (type) {
        case ResourceType::TEXTURE:
            resource = new ResourceTexture(uid);
            break;
        case ResourceType::MESH:
            resource = new ResourceMesh(uid);
            break;
        case ResourceType::MODEL:
            resource = new ResourceModel(uid);
            break;
        default:
            std::cerr << "[ModuleResources] Unsupported resource type" << std::endl;
            return nullptr;
        }

        if (resource) {
            resource->SetAssetFile(assetsFile);
        }

        return resource;
    }

    ResourceType ModuleResources::GetResourceTypeFromExtension(const std::string& ext) const {
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
            ext == ".bmp" || ext == ".tga" || ext == ".dds") {
            return ResourceType::TEXTURE;
        }
        else if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".dae") {
            return ResourceType::MODEL;
        }
        else if (ext == ".wzm") {
            return ResourceType::MESH;
        }
        else if (ext == ".json") {
            return ResourceType::SCENE;
        }

        return ResourceType::UNKNOWN;
    }

    bool ModuleResources::ProcessFile(const std::string& filepath) {
        // Check if already exists
        if (Find(filepath) != 0) {
            return false; // Already imported
        }

        // Import
        uint64_t uid = ImportFile(filepath);
        return uid != 0;
    }

} // namespace WizardEngine