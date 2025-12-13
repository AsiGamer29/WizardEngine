#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include "MetaFile.h"

namespace WizardEngine {

    class AssetManager {
    public:
        AssetManager();
        ~AssetManager();

        bool Initialize();

        // Process a file dropped/added to Assets folder
        bool ProcessAssetFile(const std::string& filepath);

        // Check if reimport is needed
        bool NeedsReimport(const std::string& assetPath);

        // Get library path for an asset
        std::string GetLibraryPath(const std::string& assetPath);

        // Get meta data for an asset
        AssetMetaData* GetMetaData(const std::string& assetPath);

        // Scan Assets folder
        void ScanAssetsFolder();

        // Statistics
        void PrintStatistics();

    private:
        std::string assetsDir;
        std::string libraryDir;

        // Cache of metadata (keyed by source file path)
        std::unordered_map<std::string, AssetMetaData> metaCache;

        bool ProcessModelFile(const std::string& filepath);
        bool ProcessTextureFile(const std::string& filepath);

        std::string GetFileExtension(const std::string& filepath);
        uint64_t GetFileTimestamp(const std::string& filepath);
    };

} // namespace WizardEngine