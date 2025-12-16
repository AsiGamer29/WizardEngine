#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <assimp/scene.h>

// Forward declarations
struct aiMesh;

namespace WizardEngine {

    struct WizardVertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoords;

    };

    struct WizardMeshData {
        std::vector<WizardVertex> vertices;
        std::vector<unsigned int> indices;
        glm::vec3 aabbMin;
        glm::vec3 aabbMax;
    };

    class MeshImporter {
    public:
        // Import from Assimp mesh to our custom structure
        static WizardMeshData Import(const aiMesh* mesh);

        // Save our mesh to custom .wzm format
        static bool Save(const WizardMeshData& meshData, const std::string& filepath);

        // Load mesh from custom .wzm format
        static bool Load(const std::string& filepath, WizardMeshData& outMeshData);

        // Get file size for logging purposes
        static size_t GetFileSize(const std::string& filepath);

    private:
        struct WizardMeshHeader {
            char magic[4];          // "WZM\0"
            unsigned int version;   // Format version
            unsigned int vertexCount;
            unsigned int indexCount;
            glm::vec3 aabbMin;
            glm::vec3 aabbMax;
        };

        static void CalculateAABB(WizardMeshData& meshData);
    };

} // namespace WizardEngine