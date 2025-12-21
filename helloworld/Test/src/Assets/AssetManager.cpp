#include "AssetManager.h"
#include "MeshImporter.h"
#include "TextureImporter.h"
#include "ModelImporter.h"
#include <iostream>
#include <fstream>
#include <algorithm>

namespace WizardEngine {

    AssetManager::AssetManager()
        : assetsDir("Assets/"), libraryDir("Library/") {
    }

    AssetManager::~AssetManager() {
    }

    bool AssetManager::Initialize() {
        std::cout << "[AssetManager] Initializing..." << std::endl;

        std::filesystem::create_directories(assetsDir);
        std::filesystem::create_directories(libraryDir);
        std::filesystem::create_directories(libraryDir + "Meshes");
        std::filesystem::create_directories(libraryDir + "Textures");
        std::filesystem::create_directories(libraryDir + "Materials");
        std::filesystem::create_directories(libraryDir + "Models");

        std::cout << "[AssetManager] Directory structure created" << std::endl;

        ScanAssetsFolder();

        return true;
    }

    uint64_t AssetManager::GetFileTimestamp(const std::string& filepath) {
        if (!std::filesystem::exists(filepath)) return 0;

        auto ftime = std::filesystem::last_write_time(filepath);
        return ftime.time_since_epoch().count();
    }

    bool AssetManager::ProcessAssetFile(const std::string& filepath) {
        std::string ext = GetFileExtension(filepath);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        // Check if .meta exists
        std::string metaPath = MetaFile::GetMetaPath(filepath);
        bool metaExists = std::filesystem::exists(metaPath);

        AssetMetaData metaData;

        if (metaExists) {
            // Load existing meta silently
            if (!MetaFile::LoadSilent(metaPath, metaData)) {
                std::cerr << "[AssetManager] Failed to load meta, will recreate" << std::endl;
                metaExists = false;
            }
        }

        if (!metaExists) {
            // Create new meta
            metaData.sourceFile = filepath;
            metaData.uuid = MetaFile::GenerateUUID();
            metaData.sourceTimestamp = GetFileTimestamp(filepath);
            metaData.lastImportTimestamp = 0; // Force import
        }

        // Check if needs reimport
        uint64_t currentTimestamp = GetFileTimestamp(filepath);
        bool needsImport = (currentTimestamp != metaData.sourceTimestamp) ||
            (metaData.lastImportTimestamp == 0) ||
            (metaData.libraryFile.empty()); // CRITICAL FIX: Also reimport if libraryFile is missing

        if (!needsImport) {
            // Silently cache and skip
            metaCache[filepath] = metaData;
            return true;
        }

        std::cout << "[AssetManager] Processing: " << filepath << std::endl;

        // CRITICAL FIX: Pre-generate library paths based on asset type
        std::string libraryPath;

        // Determine asset type and generate library path
        if (ext == "fbx" || ext == "obj" || ext == "gltf" || ext == "glb" || ext == "dae") {
            metaData.assetType = "model";
            std::string outputDir = libraryDir + "Models/" +
                std::filesystem::path(filepath).stem().string();
            libraryPath = outputDir + "/model.wzd";
            metaData.libraryFile = libraryPath; // Set BEFORE processing

            if (!ProcessModelFile(filepath)) {
                return false;
            }
        }
        else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" ||
            ext == "tga" || ext == "dds") {
            metaData.assetType = "texture";
            std::string filename = std::filesystem::path(filepath).stem().string() + ".wzt";
            libraryPath = libraryDir + "Textures/" + filename;
            metaData.libraryFile = libraryPath; // Set BEFORE processing

            if (!ProcessTextureFile(filepath)) {
                return false;
            }
        }
        else {
            std::cout << "[AssetManager] Unsupported file type: " << ext << std::endl;
            return false;
        }

        // CRITICAL FIX: Verify libraryFile is set and file exists
        if (metaData.libraryFile.empty()) {
            std::cerr << "[AssetManager] ERROR: libraryFile is EMPTY after processing!" << std::endl;
            std::cerr << "[AssetManager] Asset type: " << metaData.assetType << std::endl;
            std::cerr << "[AssetManager] Source: " << filepath << std::endl;
            return false;
        }

        if (!std::filesystem::exists(metaData.libraryFile)) {
            std::cerr << "[AssetManager] ERROR: Library file was not created: " << metaData.libraryFile << std::endl;
            return false;
        }

        // Update meta timestamps
        metaData.sourceTimestamp = currentTimestamp;
        metaData.lastImportTimestamp = GetFileTimestamp(filepath);

        // Save meta file
        if (!MetaFile::Save(metaPath, metaData)) {
            std::cerr << "[AssetManager] ERROR: Failed to save meta file!" << std::endl;
            return false;
        }

        // Update cache with final metadata
        metaCache[filepath] = metaData;

        std::cout << "[AssetManager] Successfully processed: " << filepath << std::endl;
        std::cout << "[AssetManager] Library file: " << metaData.libraryFile << std::endl;

        return true;
    }

    bool AssetManager::ProcessModelFile(const std::string& filepath) {
        std::cout << "[AssetManager] Importing model: " << filepath << std::endl;

        auto startTotal = std::chrono::high_resolution_clock::now();

        std::string outputDir = libraryDir + "Models/" +
            std::filesystem::path(filepath).stem().string();

        WizardModelData modelData = ModelImporter::Import(filepath, outputDir);

        if (modelData.meshes.empty()) {
            std::cerr << "[AssetManager] Failed to import model: " << filepath << std::endl;
            return false;
        }

        std::string wzdPath = outputDir + "/model.wzd";

        if (!ModelImporter::Save(modelData, wzdPath)) {
            std::cerr << "[AssetManager] Failed to save WZD file: " << wzdPath << std::endl;
            return false;
        }

        // Verify the file was actually created
        if (!std::filesystem::exists(wzdPath)) {
            std::cerr << "[AssetManager] ERROR: WZD file not found after save: " << wzdPath << std::endl;
            return false;
        }

        std::cout << "[AssetManager] Model saved to: " << wzdPath << std::endl;

        auto endTotal = std::chrono::high_resolution_clock::now();
        double totalTime = std::chrono::duration<double>(endTotal - startTotal).count();

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

        WizardTextureData texData = TextureImporter::Import(filepath);

        if (texData.data.empty()) {
            std::cerr << "[AssetManager] Failed to import texture: " << filepath << std::endl;
            return false;
        }

        auto endImport = std::chrono::high_resolution_clock::now();

        std::string filename = std::filesystem::path(filepath).stem().string() + ".wzt";
        std::string wztPath = libraryDir + "Textures/" + filename;

        auto startSave = std::chrono::high_resolution_clock::now();

        if (!TextureImporter::Save(texData, wztPath)) {
            std::cerr << "[AssetManager] Failed to save WZT file: " << wztPath << std::endl;
            return false;
        }

        auto endSave = std::chrono::high_resolution_clock::now();

        // Verify the file was actually created
        if (!std::filesystem::exists(wztPath)) {
            std::cerr << "[AssetManager] ERROR: WZT file not found after save: " << wztPath << std::endl;
            return false;
        }

        std::cout << "[AssetManager] Texture saved to: " << wztPath << std::endl;

        WizardTextureData loadTest;
        auto startLoad = std::chrono::high_resolution_clock::now();
        TextureImporter::Load(wztPath, loadTest);
        auto endLoad = std::chrono::high_resolution_clock::now();

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
        std::string metaPath = MetaFile::GetMetaPath(assetPath);

        if (!std::filesystem::exists(metaPath)) {
            return true; // No meta = needs import
        }

        // Check cache first
        auto it = metaCache.find(assetPath);
        if (it != metaCache.end()) {
            uint64_t currentTimestamp = GetFileTimestamp(assetPath);
            return currentTimestamp != it->second.sourceTimestamp;
        }

        // Load silently if not in cache
        AssetMetaData metaData;
        if (!MetaFile::LoadSilent(metaPath, metaData)) {
            return true; // Can't load meta = needs import
        }

        // Cache for future use
        metaCache[assetPath] = metaData;

        uint64_t currentTimestamp = GetFileTimestamp(assetPath);
        return currentTimestamp != metaData.sourceTimestamp;
    }

    std::string AssetManager::GetLibraryPath(const std::string& assetPath) {
        auto it = metaCache.find(assetPath);
        if (it != metaCache.end()) {
            return it->second.libraryFile;
        }

        // Try to load from meta file silently
        std::string metaPath = MetaFile::GetMetaPath(assetPath);
        if (std::filesystem::exists(metaPath)) {
            AssetMetaData metaData;
            if (MetaFile::LoadSilent(metaPath, metaData)) {
                metaCache[assetPath] = metaData;
                return metaData.libraryFile;
            }
        }

        return "";
    }

    AssetMetaData* AssetManager::GetMetaData(const std::string& assetPath) {
        auto it = metaCache.find(assetPath);
        if (it != metaCache.end()) {
            return &it->second;
        }

        std::string metaPath = MetaFile::GetMetaPath(assetPath);
        if (std::filesystem::exists(metaPath)) {
            AssetMetaData metaData;
            if (MetaFile::LoadSilent(metaPath, metaData)) {
                metaCache[assetPath] = metaData;
                return &metaCache[assetPath];
            }
        }

        return nullptr;
    }

    void AssetManager::ScanAssetsFolder() {
        std::cout << "[AssetManager] Scanning Assets folder..." << std::endl;

        int processedCount = 0;
        int skippedCount = 0;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(assetsDir)) {
            if (!entry.is_regular_file()) continue;

            std::string filepath = entry.path().string();

            // Skip .meta files
            if (GetFileExtension(filepath) == "meta") continue;

            if (NeedsReimport(filepath)) {
                if (ProcessAssetFile(filepath)) {
                    processedCount++;
                }
            }
            else {
                // Load meta into cache silently
                std::string metaPath = MetaFile::GetMetaPath(filepath);
                if (std::filesystem::exists(metaPath)) {
                    AssetMetaData metaData;
                    if (MetaFile::LoadSilent(metaPath, metaData)) {
                        metaCache[filepath] = metaData;
                    }
                }
                skippedCount++;
            }
        }

        std::cout << "[AssetManager] Scan complete: "
            << processedCount << " processed, "
            << skippedCount << " skipped" << std::endl;
    }

    void AssetManager::PrintStatistics() {
        int modelCount = 0;
        int textureCount = 0;
        int otherCount = 0;

        for (const auto& pair : metaCache) {
            if (pair.second.assetType == "model") modelCount++;
            else if (pair.second.assetType == "texture") textureCount++;
            else otherCount++;
        }

        std::cout << "\n========================================" << std::endl;
        std::cout << "[AssetManager] DATABASE STATISTICS" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Total Assets: " << metaCache.size() << std::endl;
        std::cout << "Models: " << modelCount << std::endl;
        std::cout << "Textures: " << textureCount << std::endl;
        std::cout << "Other: " << otherCount << std::endl;
        std::cout << "========================================\n" << std::endl;
    }

    std::string AssetManager::GetFileExtension(const std::string& filepath) {
        size_t pos = filepath.find_last_of('.');
        if (pos != std::string::npos && pos + 1 < filepath.size()) {
            return filepath.substr(pos + 1);
        }
        return "";
    }

} // namespace WizardEngine