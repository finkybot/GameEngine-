// ***** AssetManager.h - Header file for AssetManager class *****
#pragma once
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>

namespace fs = std::filesystem; // Namespace for standard template filesystem

/// AssetManager class responsible for loading and managing game assets such as textures, sounds, etc.
class AssetManager {
public:
    // Enforce explicit construction of AssetManager
    explicit AssetManager(const std::string& rootPath) : root(rootPath) {}

    std::vector<fs::path> listAssets(const std::string& subDir, const std::string& ext) const {
		std::vector<fs::path> assets; // Vector to hold the paths of the assets found
		fs::path dir = root / subDir; // Construct the full path to the subdirectory containing the assets

		//Check if the directory exists (and is a directory) before attempting to iterate through it
        if (fs::exists(dir) && fs::is_directory(dir)) {

			// Recursively iterate through the directory and its subdirectories to find files with the specified extension
            for (auto& entry : fs::recursive_directory_iterator(dir)) {
                
				// Check if the current entry is a regular file and has the specified extension before adding it to the assets vector
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