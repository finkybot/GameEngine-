// ***** AssetManager.h - Header file for AssetManager class *****
#pragma once
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>

namespace fs = std::filesystem; // Alias for filesystem namespace to simplify code readability

/// AssetManager class responsible for loading and managing game assets such as textures, sounds, etc.
class AssetManager {
public:
    explicit AssetManager(const std::string& rootPath) : root(rootPath) {}

    std::vector<fs::path> listAssets(const std::string& subDir, const std::string& ext) const {
        std::vector<fs::path> assets;
        fs::path dir = root / subDir;

        if (fs::exists(dir) && fs::is_directory(dir)) {
            for (auto& entry : fs::recursive_directory_iterator(dir)) {
                if (entry.path().extension() == ext) {
                    assets.push_back(entry.path());
                }
            }
        }
        return assets;
    }

private:
    fs::path root;
};