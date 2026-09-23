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
//	|	AssetManager class responsible for loading and managing game assets such as textures, sounds, etc.
//	|_______________________________________________________________________
class AssetManager {
public:
	explicit AssetManager(const std::string& rootPath) : root(rootPath) {} // Constructor that initializes the AssetManager with a specified root path for asset storage. The root path is used as the base directory for all asset loading operations.

	// List assets in a specified subdirectory with a given file extension. This method recursively searches the specified subdirectory and its subdirectories for files matching the provided extension, and returns a vector of paths to the found assets.
	std::vector<fs::path> listAssets(const std::string& subDir, const std::string& ext) const {
		// Create a vector to hold the paths of the found assets
		std::vector<fs::path> assets;

		// Construct the full path to the subdirectory containing the assets by combining the root path with the specified subdirectory
		fs::path dir = root / subDir;

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
/////////////////////////////////