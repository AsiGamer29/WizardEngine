#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>

// Forward declarations
struct aiScene;
struct aiNode;

namespace WizardEngine {

    struct WizardMeshReference {
        std::string meshFilepath;    // Path to .wzm file
        unsigned int materialIndex;
    };

    struct WizardMaterialData {
        std::string name;
        std::string diffuseTexture;  // Path to .wzt file
        glm::vec3 diffuseColor;
        glm::vec3 specularColor;
        float shininess;
    };

    struct WizardNodeData {
        std::string name;
        glm::mat4 transformation;
        int parentIndex;             // -1 if root
        std::vector<int> childIndices;
        std::vector<int> meshIndices; // Indices into model's mesh list
    };

    struct WizardModelData {
        std::vector<WizardMeshReference> meshes;
        std::vector<WizardMaterialData> materials;
        std::vector<WizardNodeData> nodes;
        int rootNodeIndex;
    };

    class ModelImporter {
    public:
        // Import full model from FBX/OBJ using Assimp
        static WizardModelData Import(const std::string& filepath,
            const std::string& outputDirectory);

        // Save model metadata to custom .wzd format
        static bool Save(const WizardModelData& modelData, const std::string& filepath);

        // Load model metadata from custom .wzd format
        static bool Load(const std::string& filepath, WizardModelData& outModelData);

        // Get timing information
        struct ImportTiming {
            double fbxLoadTime;
            double customSaveTime;
            double customLoadTime;
            size_t fbxFileSize;
            size_t customFileSize;
        };

        static ImportTiming GetLastImportTiming() { return lastTiming; }

    private:
        static ImportTiming lastTiming;

        struct WizardModelHeader {
            char magic[4];              // "WZD\0"
            unsigned int version;       // Format version
            unsigned int meshCount;
            unsigned int materialCount;
            unsigned int nodeCount;
            int rootNodeIndex;
        };

        static void ProcessNode(const aiScene* scene, aiNode* node,
            WizardModelData& modelData, int parentIndex,
            const std::string& outputDirectory);

        static std::string ExtractFilename(const std::string& path);
    };

} // namespace WizardEngine