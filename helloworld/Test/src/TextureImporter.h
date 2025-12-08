#pragma once
#include <string>
#include <vector>

namespace WizardEngine {

    struct WizardTextureData {
        unsigned int width;
        unsigned int height;
        unsigned char channels;
        std::vector<unsigned char> data;
    };

    class TextureImporter {
    public:
        // Import from image file using DevIL
        static WizardTextureData Import(const std::string& filepath);

        // Save texture to custom .wzt format
        static bool Save(const WizardTextureData& textureData, const std::string& filepath);

        // Load texture from custom .wzt format
        static bool Load(const std::string& filepath, WizardTextureData& outTextureData);

        // Get file size for logging purposes
        static size_t GetFileSize(const std::string& filepath);

    private:
        struct WizardTextureHeader {
            char magic[4];          // "WZT\0"
            unsigned int version;   // Format version
            unsigned int width;
            unsigned int height;
            unsigned char channels;
            unsigned char padding[3]; // Alignment
        };
    };

}