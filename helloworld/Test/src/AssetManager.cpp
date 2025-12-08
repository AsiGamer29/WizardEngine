#include "AssetManager.h"
#include "MeshImporter.h"
#include "TextureImporter.h"
#include "ModelImporter.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace WizardEngine {

    AssetManager::AssetManager()
        : assetsDir("Assets/"), libraryDir("Library/") {
    }

    AssetManager::~AssetManager() {
        SaveMetadata();
    }

    bool AssetManager::Initialize() {
        std::cout << "[AssetManager] Initializing..." << std::endl;

        // Create main directories
        std::filesystem::create_directories(assetsDir);
        std::filesystem::create_directories(libraryDir);

        // Create library subdirectories
        std::filesystem::create_directories(libraryDir + "Meshes");
        std::filesystem::create_directories(libraryDir + "Textures");
        std::filesystem::create_directories(libraryDir + "Materials");
        std::filesystem::create_directories(libraryDir + "Models");

        std::cout << "[AssetManager] Created directory structure:" << std::endl;
        std::cout << "  - " << assetsDir << std::endl;
        std::cout << "  - " << libraryDir << "Meshes" << std::endl;
        std::cout << "  - " << libraryDir << "Textures" << std::endl;
        std::cout << "  - " << libraryDir << "Materials" << std::endl;
        std::cout << "  - " << libraryDir << "Models" << std::endl;

        LoadMetadata();
        ScanAssetsFolder();

        return true;
    }

    bool AssetManager::ProcessAssetFile(const std::string& filepath) {
        std::string ext = GetFileExtension(filepath);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        std::cout << "[AssetManager] Processing: " << filepath << std::endl;

        // Model formats
        if (ext == "fbx" || ext == "obj" || ext == "gltf" || ext == "glb" || ext == "dae") {
            return ProcessModelFile(filepath);
        }
        // Texture formats
        else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" ||
            ext == "tga" || ext == "dds") {
            return ProcessTextureFile(filepath);
        }

        std::cout << "[AssetManager] Unsupported file type: " << ext << std::endl;
        return false;
    }

    bool AssetManager::ProcessModelFile(const std::string& filepath) {
        std::cout << "[AssetManager] Importing model: " << filepath << std::endl;

        auto startTotal = std::chrono::high_resolution_clock::now();

        // Import model to custom format
        std::string outputDir = libraryDir + "Models/" +
            std::filesystem::path(filepath).stem().string();

        WizardModelData modelData = ModelImporter::Import(filepath, outputDir);

        if (modelData.meshes.empty()) {
            std::cerr << "[AssetManager] Failed to import model: " << filepath << std::endl;
            return false;
        }

        // Save model metadata
        std::string wzdPath = outputDir + "/model.wzd";
        ModelImporter::Save(modelData, wzdPath);

        auto endTotal = std::chrono::high_resolution_clock::now();
        double totalTime = std::chrono::duration<double>(endTotal - startTotal).count();

        // Update metadata
        AssetMetadata meta;
        meta.sourcePath = filepath;
        meta.libraryPath = wzdPath;
        meta.lastModified = std::filesystem::last_write_time(filepath);
        meta.type = "model";
        assetDatabase[filepath] = meta;

        // Get timing info
        auto timing = ModelImporter::GetLastImportTiming();

        std::cout << "\n========================================" << std::endl;
        std::cout << "[AssetManager] IMPORT STATISTICS" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Source File: " << filepath << std::endl;
        std::cout << "FBX Size: " << timing.fbxFileSize << " bytes" << std::endl;
        std::cout << "WZD Size: " << timing.customFileSize << " bytes" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "FBX Load Time: " << timing.fbxLoadTime << "s" << std::endl;
        std::cout << "Custom Save Time: " << timing.customSaveTime << "s" << std::endl;
        std::cout << "Custom Load Time: " << timing.customLoadTime << "s" << std::endl;
        std::cout << "Total Time: " << totalTime << "s" << std::endl;
        std::cout << "----------------------------------------" << std::endl;

        if (timing.fbxLoadTime > 0) {
            double speedup = timing.fbxLoadTime / timing.customLoadTime;
            std::cout << "SPEEDUP: " << speedup << "x faster!" << std::endl;
        }

        std::cout << "========================================\n" << std::endl;

        return true;
    }

    bool AssetManager::ProcessTextureFile(const std::string& filepath) {
        std::cout << "[AssetManager] Importing texture: " << filepath << std::endl;

        auto startImport = std::chrono::high_resolution_clock::now();

        // Import texture
        WizardTextureData texData = TextureImporter::Import(filepath);

        if (texData.data.empty()) {
            std::cerr << "[AssetManager] Failed to import texture: " << filepath << std::endl;
            return false;
        }

        auto endImport = std::chrono::high_resolution_clock::now();

        // Save to library
        std::string filename = std::filesystem::path(filepath).stem().string() + ".wzt";
        std::string wztPath = libraryDir + "Textures/" + filename;

        auto startSave = std::chrono::high_resolution_clock::now();
        TextureImporter::Save(texData, wztPath);
        auto endSave = std::chrono::high_resolution_clock::now();

        // Load test
        WizardTextureData loadTest;
        auto startLoad = std::chrono::high_resolution_clock::now();
        TextureImporter::Load(wztPath, loadTest);
        auto endLoad = std::chrono::high_resolution_clock::now();

        // Update metadata
        AssetMetadata meta;
        meta.sourcePath = filepath;
        meta.libraryPath = wztPath;
        meta.lastModified = std::filesystem::last_write_time(filepath);
        meta.type = "texture";
        assetDatabase[filepath] = meta;

        // Print statistics
        double importTime = std::chrono::duration<double>(endImport - startImport).count();
        double saveTime = std::chrono::duration<double>(endSave - startSave).count();
        double loadTime = std::chrono::duration<double>(endLoad - startLoad).count();

        size_t originalSize = std::filesystem::file_size(filepath);
        size_t customSize = TextureImporter::GetFileSize(wztPath);

        std::cout << "\n========================================" << std::endl;
        std::cout << "[AssetManager] TEXTURE STATISTICS" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Source: " << filepath << std::endl;
        std::cout << "Original Size: " << originalSize << " bytes" << std::endl;
        std::cout << "WZT Size: " << customSize << " bytes" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Import Time: " << importTime << "s" << std::endl;
        std::cout << "Save Time: " << saveTime << "s" << std::endl;
        std::cout << "Load Time: " << loadTime << "s" << std::endl;
        std::cout << "========================================\n" << std::endl;

        return true;
    }

    bool AssetManager::NeedsReimport(const std::string& assetPath) {
        auto it = assetDatabase.find(assetPath);
        if (it == assetDatabase.end()) {
            return true; // Not in database, needs import
        }

        if (!std::filesystem::exists(assetPath)) {
            return false; // Source doesn't exist
        }

        if (!std::filesystem::exists(it->second.libraryPath)) {
            return true; // Library file missing
        }

        auto currentModified = std::filesystem::last_write_time(assetPath);
        return currentModified > it->second.lastModified;
    }

    std::string AssetManager::GetLibraryPath(const std::string& assetPath,
        const std::string& extension) {
        auto it = assetDatabase.find(assetPath);
        if (it != assetDatabase.end()) {
            return it->second.libraryPath;
        }
        return "";
    }

    void AssetManager::ScanAssetsFolder() {
        std::cout << "[AssetManager] Scanning Assets folder..." << std::endl;

        int processedCount = 0;
        int skippedCount = 0;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(assetsDir)) {
            if (!entry.is_regular_file()) continue;

            std::string filepath = entry.path().string();

            if (NeedsReimport(filepath)) {
                if (ProcessAssetFile(filepath)) {
                    processedCount++;
                }
            }
            else {
                skippedCount++;
            }
        }

        std::cout << "[AssetManager] Scan complete: "
            << processedCount << " processed, "
            << skippedCount << " skipped" << std::endl;
    }

    std::vector<std::string> AssetManager::GetAssetsByExtension(const std::string& extension) {
        std::vector<std::string> results;

        for (const auto& pair : assetDatabase) {
            std::string ext = GetFileExtension(pair.first);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == extension) {
                results.push_back(pair.first);
            }
        }

        return results;
    }

    void AssetManager::PrintStatistics() {
        int modelCount = 0;
        int textureCount = 0;
        int otherCount = 0;

        for (const auto& pair : assetDatabase) {
            if (pair.second.type == "model") modelCount++;
            else if (pair.second.type == "texture") textureCount++;
            else otherCount++;
        }

        std::cout << "\n========================================" << std::endl;
        std::cout << "[AssetManager] DATABASE STATISTICS" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Total Assets: " << assetDatabase.size() << std::endl;
        std::cout << "Models: " << modelCount << std::endl;
        std::cout << "Textures: " << textureCount << std::endl;
        std::cout << "Other: " << otherCount << std::endl;
        std::cout << "========================================\n" << std::endl;
    }

    void AssetManager::SaveMetadata() {
        nlohmann::json j;

        for (const auto& pair : assetDatabase) {
            nlohmann::json asset;
            asset["sourcePath"] = pair.second.sourcePath;
            asset["libraryPath"] = pair.second.libraryPath;
            asset["type"] = pair.second.type;

            auto timepoint = pair.second.lastModified.time_since_epoch().count();
            asset["lastModified"] = timepoint;

            j[pair.first] = asset;
        }

        std::ofstream file(libraryDir + "metadata.json");
        if (file.is_open()) {
            file << j.dump(4);
            file.close();
            std::cout << "[AssetManager] Metadata saved" << std::endl;
        }
    }

    void AssetManager::LoadMetadata() {
        std::string metaPath = libraryDir + "metadata.json";

        if (!std::filesystem::exists(metaPath)) {
            std::cout << "[AssetManager] No metadata found, starting fresh" << std::endl;
            return;
        }

        std::ifstream file(metaPath);
        if (!file.is_open()) {
            std::cerr << "[AssetManager] Failed to load metadata" << std::endl;
            return;
        }

        nlohmann::json j;
        file >> j;
        file.close();

        for (auto& item : j.items()) {
            AssetMetadata meta;
            meta.sourcePath = item.value()["sourcePath"];
            meta.libraryPath = item.value()["libraryPath"];
            meta.type = item.value()["type"];

            auto timepoint = item.value()["lastModified"].get<long long>();
            meta.lastModified = std::filesystem::file_time_type(
                std::filesystem::file_time_type::duration(timepoint)
            );

            assetDatabase[item.key()] = meta;
        }

        std::cout << "[AssetManager] Loaded metadata: "
            << assetDatabase.size() << " assets" << std::endl;
    }

    std::string AssetManager::GetFileExtension(const std::string& filepath) {
        size_t pos = filepath.find_last_of('.');
        if (pos != std::string::npos && pos + 1 < filepath.size()) {
            return filepath.substr(pos + 1);
        }
        return "";
    }

    std::string AssetManager::GetRelativePath(const std::string& fullPath,
        const std::string& basePath) {
        std::filesystem::path full(fullPath);
        std::filesystem::path base(basePath);
        return std::filesystem::relative(full, base).string();
    }

} // namespace WizardEngine