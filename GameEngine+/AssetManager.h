/////////////////////////////////
// AssetManager.h
/////////////////////////////////



/////////////////////////////////
// Includes and necessary headers for the AssetManager class.
#pragma once
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
/////////////////////////////////



/////////////////////////////////
// Namespace alias for filesystem to simplify code and improve readability when working with file paths and directories in the AssetManager class.
namespace fs = std::filesystem;
/////////////////////////////////



/////////////////////////////////
/// AssetManager class responsible for loading and managing game assets such as textures, sounds, etc.
class AssetManager {
	/////////////////////////////////
	// Public interface for the AssetManager class
public:
	/////////////////////////////////
	// Constructor for the AssetManager class. Initializes the asset manager with a specified root path where assets are stored. This root path will be 
	// used as the base directory for loading assets, allowing for organized asset management and easy retrieval of assets based on their relative paths within the root directory.
	// Enforced to be explicit to prevent accidental implicit conversions from string literals, which could lead to unintended behavior when creating an AssetManager instance.
	explicit AssetManager(const std::string& rootPath) : root(rootPath) {}
	/////////////////////////////////



	/////////////////////////////////
	// ListAssets - lists all assets in a specified subdirectory with a given file extension. This method recursively searches through the specified subdirectory within the root directory and 
	// collects paths of all files that match the specified extension, allowing for easy retrieval of assets based on their type (e.g., ".png" for textures, ".wav" for sounds) and organization within the asset directory structure.
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
	/////////////////////////////////



	/////////////////////////////////
	// Private member variable/s for the AssetManager class
private:
	/////////////////////////////////
	// Root path for asset storage. This is the base directory where all assets are stored, and it will be used as a prefix for all asset loading operations.
	fs::path root;
	/////////////////////////////////
};
/////////////////////////////////