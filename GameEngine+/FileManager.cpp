/////////////////////////////////
// FileManager.cpp - Implementation of centralized file management system
/////////////////////////////////



/////////////////////////////////
// Includes
#include "FileManager.h"
#include "TileMap.h"
#include <iostream>
/////////////////////////////////



/////////////////////////////////
// Constructor
FileManager::FileManager(const std::filesystem::path& basePath) 
	: m_basePath(basePath) {}
/////////////////////////////////



/////////////////////////////////
// TILEMAP LOADING
/////////////////////////////////
/////////////////////////////////
// LoadTileMap - loads a TileMap from a JSON file at the specified path. This function first checks if the path is empty and returns an error if 
// it is. It then resolves the path against the base path of the FileManager, checks if the tile map is already cached, and if not, it attempts 
// to load it from the file using TileMap's own loader. If loading is successful, it caches the tile map for future use and returns it; otherwise, 
// it returns an error message.
std::optional<TileMap> FileManager::LoadTileMap(const std::string& path, std::string* outErr)
{
	if (path.empty()) {
		if (outErr) *outErr = "Path is empty";
		return std::nullopt;
	}

	// Resolve the path
	std::filesystem::path resolvedPath = ResolvePath(path);

	// Check cache first
	std::string cacheKey = resolvedPath.string();
	auto it = m_tileMapCache.find(cacheKey);
	if (it != m_tileMapCache.end()) {
		std::cout << "TileMap loaded from cache: " << cacheKey << std::endl;
		return it->second;
	}

	// Load from file using TileMap's own loader
	auto maybeMap = TileMap::LoadFromJSON(cacheKey, outErr);
	if (maybeMap) {
		// Cache the result
		m_tileMapCache[cacheKey] = *maybeMap;
		std::cout << "TileMap loaded and cached: " << cacheKey << std::endl;
		return maybeMap;
	}

	return std::nullopt;
}
/////////////////////////////////



/////////////////////////////////
// CACHE MANAGEMENT
/////////////////////////////////
/////////////////////////////////
// ClearCache - clears all cached files from memory, including tile maps, textures, fonts, etc. This is a comprehensive cache management 
// function that can be used when we want to free up memory or ensure that all resources are reloaded fresh on the next access. It currently 
// only clears the tile map cache, but can be extended in the future to clear other types of caches as needed.
void FileManager::ClearCache()
{
	m_tileMapCache.clear();
	std::cout << "FileManager: All caches cleared" << std::endl;
}
/////////////////////////////////



/////////////////////////////////
// ClearTileMapCache - clears only the tile map cache, leaving other caches (e.g. textures, fonts) intact. This allows for more targeted	
// cache management when we know only tile maps need to be reloaded, without affecting other resources that may still be valid.
void FileManager::ClearTileMapCache()
{
	m_tileMapCache.clear();
	std::cout << "FileManager: TileMap cache cleared" << std::endl;
}
/////////////////////////////////



/////////////////////////////////
// PATH UTILITIES
/////////////////////////////////
/////////////////////////////////
// ResolvePath - this function takes a file path as input and resolves it against the base path of the FileManager. If the input path is absolute, 
// it is returned as-is. If it is relative, it is combined with the base path to produce an absolute path. This allows for flexible file loading 
// where users can specify either absolute or relative paths, and the FileManager will handle the resolution based on its configured base path.
std::filesystem::path FileManager::ResolvePath(const std::string& path) const
{
	std::filesystem::path p(path);

	// If absolute path, use as-is
	if (p.is_absolute()) {
		return p;
	}

	// If relative, resolve against base path
	return m_basePath / p;
}
/////////////////////////////////



/////////////////////////////////
// SetBasePath - this function updates the base path used for resolving relative file paths. When the base path is changed, it will affect how all subsequent 
// relative paths are resolved, allowing for dynamic changes to the file loading context (e.g., changing the working directory for asset loading). This can be 
// useful in scenarios where we want to load files from different directories without having to specify absolute paths for each file.
void FileManager::SetBasePath(const std::filesystem::path& basePath)
{
	m_basePath = basePath;
	std::cout << "FileManager: Base path set to " << basePath.string() << std::endl;
}
/////////////////////////////////



/////////////////////////////////
// GetBasePath - this function returns the current base path that the FileManager uses for resolving relative paths. This allows other parts of the engine or
// application to query the current base path and make decisions based on it, such as constructing absolute paths for file operations or displaying the current
// working directory to the user.
std::filesystem::path FileManager::GetBasePath() const
{
	return m_basePath;
}
/////////////////////////////////



/////////////////////////////////
// FILE VALIDATION
/////////////////////////////////
/////////////////////////////////
// FileExists - this function checks if a file exists at the given path. It first resolves the path using the ResolvePath function to ensure that relative paths are 
// correctly handled, and then uses std::filesystem to check if the resolved path exists and is a regular file (not a directory). This is a common utility function 
// that can be used throughout the engine to validate file paths before attempting to load resources.
bool FileManager::FileExists(const std::string& path) const
{
	std::filesystem::path resolvedPath = ResolvePath(path);
	return std::filesystem::exists(resolvedPath) && std::filesystem::is_regular_file(resolvedPath);
}
/////////////////////////////////
