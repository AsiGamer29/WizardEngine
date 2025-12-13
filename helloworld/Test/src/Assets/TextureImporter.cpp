#include "TextureImporter.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <IL/il.h>
#include <IL/ilu.h>
#include <filesystem>

namespace WizardEngine {

    WizardTextureData TextureImporter::Import(const std::string& filepath) {
        WizardTextureData textureData;

        ILuint imgID;
        ilGenImages(1, &imgID);
        ilBindImage(imgID);

        if (!ilLoadImage(filepath.c_str())) {
            ILenum error = ilGetError();
            std::cerr << "[TextureImporter] Failed to load: " << filepath << std::endl;
            std::cerr << "[TextureImporter] DevIL Error: " << iluErrorString(error) << std::endl;
            ilDeleteImages(1, &imgID);
            return textureData;
        }

        // Get original format information
        textureData.width = ilGetInteger(IL_IMAGE_WIDTH);
        textureData.height = ilGetInteger(IL_IMAGE_HEIGHT);
        textureData.channels = ilGetInteger(IL_IMAGE_CHANNELS);

        // Convert to RGBA for consistency
        if (!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE)) {
            ILenum error = ilGetError();
            std::cerr << "[TextureImporter] Failed to convert image: " << iluErrorString(error) << std::endl;
            ilDeleteImages(1, &imgID);
            return textureData;
        }

        // Copy pixel data
        size_t dataSize = textureData.width * textureData.height * 4; // Always RGBA
        textureData.data.resize(dataSize);
        std::memcpy(textureData.data.data(), ilGetData(), dataSize);

        ilDeleteImages(1, &imgID);

        std::cout << "[TextureImporter] Imported texture: " << filepath
            << " (" << textureData.width << "x" << textureData.height
            << ", " << (int)textureData.channels << " channels)" << std::endl;

        return textureData;
    }

    bool TextureImporter::Save(const WizardTextureData& textureData, const std::string& filepath) {
        // Create directory if it doesn't exist
        std::filesystem::path path(filepath);
        std::filesystem::create_directories(path.parent_path());

        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[TextureImporter] Failed to save: " << filepath << std::endl;
            return false;
        }

        // Write header
        WizardTextureHeader header;
        std::memcpy(header.magic, "WZT", 4);
        header.version = 1;
        header.width = textureData.width;
        header.height = textureData.height;
        header.channels = textureData.channels;
        header.padding[0] = 0;
        header.padding[1] = 0;
        header.padding[2] = 0;

        file.write(reinterpret_cast<const char*>(&header), sizeof(WizardTextureHeader));

        // Write pixel data
        file.write(reinterpret_cast<const char*>(textureData.data.data()),
            textureData.data.size());

        file.close();

        size_t fileSize = GetFileSize(filepath);
        std::cout << "[TextureImporter] Saved: " << filepath
            << " (" << fileSize << " bytes)" << std::endl;

        return true;
    }

    bool TextureImporter::Load(const std::string& filepath, WizardTextureData& outTextureData) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[TextureImporter] Failed to load: " << filepath << std::endl;
            return false;
        }

        // Read header
        WizardTextureHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(WizardTextureHeader));

        // Verify magic number
        if (std::strncmp(header.magic, "WZT", 3) != 0) {
            std::cerr << "[TextureImporter] Invalid WZT file: " << filepath << std::endl;
            file.close();
            return false;
        }

        // Verify version
        if (header.version != 1) {
            std::cerr << "[TextureImporter] Unsupported version: " << header.version << std::endl;
            file.close();
            return false;
        }

        // Read texture data
        outTextureData.width = header.width;
        outTextureData.height = header.height;
        outTextureData.channels = header.channels;

        size_t dataSize = header.width * header.height * 4; // Always RGBA in file
        outTextureData.data.resize(dataSize);
        file.read(reinterpret_cast<char*>(outTextureData.data.data()), dataSize);

        file.close();

        std::cout << "[TextureImporter] Loaded: " << filepath
            << " (" << header.width << "x" << header.height
            << ", " << (int)header.channels << " channels)" << std::endl;

        return true;
    }

    size_t TextureImporter::GetFileSize(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return 0;
        size_t size = file.tellg();
        file.close();
        return size;
    }

} // namespace WizardEngine