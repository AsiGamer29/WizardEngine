#include "MetaFile.h"
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>

namespace WizardEngine {

    std::string MetaFile::GetMetaPath(const std::string& assetPath) {
        return assetPath + ".meta";
    }

    bool MetaFile::Exists(const std::string& assetPath) {
        return std::filesystem::exists(GetMetaPath(assetPath));
    }

    std::string MetaFile::GenerateUUID() {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dis;

        std::stringstream ss;
        ss << std::hex << std::setw(16) << std::setfill('0') << dis(gen);
        return ss.str();
    }

    nlohmann::json MetaFile::Serialize(const AssetMetaData& data) {
        nlohmann::json j;

        j["sourceFile"] = data.sourceFile;
        j["libraryFile"] = data.libraryFile;
        j["assetType"] = data.assetType;
        j["sourceTimestamp"] = data.sourceTimestamp;
        j["lastImportTimestamp"] = data.lastImportTimestamp;
        j["uuid"] = data.uuid;

        j["importScale"] = { data.importScale.x, data.importScale.y, data.importScale.z };
        j["importRotation"] = { data.importRotation.x, data.importRotation.y, data.importRotation.z };
        j["importPosition"] = { data.importPosition.x, data.importPosition.y, data.importPosition.z };

        j["generateColliders"] = data.generateColliders;
        j["optimizeMesh"] = data.optimizeMesh;
        j["flipUVs"] = data.flipUVs;

        return j;
    }

    AssetMetaData MetaFile::Deserialize(const nlohmann::json& json) {
        AssetMetaData data;

        if (json.contains("sourceFile")) data.sourceFile = json["sourceFile"];
        if (json.contains("libraryFile")) data.libraryFile = json["libraryFile"];
        if (json.contains("assetType")) data.assetType = json["assetType"];
        if (json.contains("sourceTimestamp")) data.sourceTimestamp = json["sourceTimestamp"];
        if (json.contains("lastImportTimestamp")) data.lastImportTimestamp = json["lastImportTimestamp"];
        if (json.contains("uuid")) data.uuid = json["uuid"];

        if (json.contains("importScale")) {
            auto scale = json["importScale"];
            data.importScale = glm::vec3(scale[0], scale[1], scale[2]);
        }

        if (json.contains("importRotation")) {
            auto rot = json["importRotation"];
            data.importRotation = glm::vec3(rot[0], rot[1], rot[2]);
        }

        if (json.contains("importPosition")) {
            auto pos = json["importPosition"];
            data.importPosition = glm::vec3(pos[0], pos[1], pos[2]);
        }

        if (json.contains("generateColliders")) data.generateColliders = json["generateColliders"];
        if (json.contains("optimizeMesh")) data.optimizeMesh = json["optimizeMesh"];
        if (json.contains("flipUVs")) data.flipUVs = json["flipUVs"];

        return data;
    }

    bool MetaFile::Save(const std::string& metaPath, const AssetMetaData& data) {
        try {
            nlohmann::json j = Serialize(data);

            std::ofstream file(metaPath);
            if (!file.is_open()) {
                std::cerr << "[MetaFile] Failed to save: " << metaPath << std::endl;
                return false;
            }

            file << j.dump(4);
            file.close();

            std::cout << "[MetaFile] Saved: " << metaPath << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[MetaFile] Error saving: " << e.what() << std::endl;
            return false;
        }
    }

    bool MetaFile::Load(const std::string& metaPath, AssetMetaData& outData) {
        try {
            std::ifstream file(metaPath);
            if (!file.is_open()) {
                std::cerr << "[MetaFile] Failed to load: " << metaPath << std::endl;
                return false;
            }

            nlohmann::json j;
            file >> j;
            file.close();

            outData = Deserialize(j);

            std::cout << "[MetaFile] Loaded: " << metaPath << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[MetaFile] Error loading: " << e.what() << std::endl;
            return false;
        }
    }

} // namespace WizardEngine