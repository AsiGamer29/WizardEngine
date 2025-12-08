#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

namespace WizardEngine {

    class AssetManager {
    public:
        AssetManager();
        ~AssetManager();

        // Initialize directories
        bool Initialize();

        // Process a dropped/added file in Assets folder
        bool ProcessAssetFile(const std::string& filepath);

        // Check if asset needs reimport (source modified)
        bool NeedsReimport(const std::string& assetPath);

        // Get library path for an asset
        std::string GetLibraryPath(const std::string& assetPath, const std::string& extension);

        // Scan Assets folder for changes
        void ScanAssetsFolder();

        // Get all assets of a specific type
        std::vector<std::string> GetAssetsByExtension(const std::string& extension);

        // Print statistics
        void PrintStatistics();

    private:
        std::string assetsDir;
        std::string libraryDir;

        struct AssetMetadata {
            std::string sourcePath;
            std::string libraryPath;
            std::filesystem::file_time_type lastModified;
            std::string type; // "mesh", "texture", "model"
        };

        std::unordered_map<std::string, AssetMetadata> assetDatabase;

        bool ProcessMeshFile(const std::string& filepath);
        bool ProcessTextureFile(const std::string& filepath);
        bool ProcessModelFile(const std::string& filepath);

        std::string GetFileExtension(const std::string& filepath);
        std::string GetRelativePath(const std::string& fullPath, const std::string& basePath);

        void SaveMetadata();
        void LoadMetadata();
    };

} // namespace WizardEngine