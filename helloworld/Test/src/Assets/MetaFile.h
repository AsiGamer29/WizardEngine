#pragma once
#include <string>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

namespace WizardEngine {

    struct AssetMetaData {
        std::string sourceFile;
        std::string libraryFile;
        std::string assetType;
        uint64_t sourceTimestamp;
        uint64_t lastImportTimestamp;

        glm::vec3 importScale = glm::vec3(1.0f);
        glm::vec3 importRotation = glm::vec3(0.0f);
        glm::vec3 importPosition = glm::vec3(0.0f);

        bool generateColliders = false;
        bool optimizeMesh = true;
        bool flipUVs = true;

        std::string uuid;
    };

    class MetaFile {
    public:
        static bool Save(const std::string& metaPath, const AssetMetaData& data);
        static bool Load(const std::string& metaPath, AssetMetaData& outData);
        static bool LoadSilent(const std::string& metaPath, AssetMetaData& outData);
        static bool Exists(const std::string& assetPath);
        static std::string GetMetaPath(const std::string& assetPath);
        static std::string GenerateUUID();

        static AssetMetaData DeserializeFromJson(const nlohmann::json& json);

    private:
        static nlohmann::json Serialize(const AssetMetaData& data);
        static AssetMetaData Deserialize(const nlohmann::json& json);
    };

} // namespace WizardEngine