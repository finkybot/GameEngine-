/////////////////////////////////
// FileManager.h - Centralized file loading and management system for all engine assets
/////////////////////////////////



/////////////////////////////////
// Includes and necessary headers for the FileManager class.
#pragma once
#include <string>
#include <optional>
#include <unordered_map>
#include <memory>
#include <filesystem>
/////////////////////////////////



/////////////////////////////////
// Forward declarations for types that FileManager loads
struct TileMap;
/////////////////////////////////



/////////////////////////////////
// FileManager class - centralized file management system for loading all types of engine assets
// (tilemaps, textures, images, configurations, etc.). Provides uniform interface across different
// file types with built-in caching, error handling, and path resolution.
class FileManager {
	/////////////////////////////////
	// Public interface
public:
	/////////////////////////////////
	// Constructor with optional base path for resolving relative paths
	explicit FileManager(const std::filesystem::path& basePath = ".");
	~FileManager() = default;
	/////////////////////////////////



	/////////////////////////////////
	// TILEMAP LOADING
	// LoadTileMap - loads a TileMap from a JSON file at the specified path
	std::optional<TileMap> LoadTileMap(const std::string& path, std::string* outErr = nullptr);
	/////////////////////////////////



	/////////////////////////////////
	// CACHE MANAGEMENT
	// ClearCache - clears all cached files from memory
	void ClearCache();
	/////////////////////////////////
	// ClearTileMapCache - clears only cached tilemaps
	void ClearTileMapCache();
	/////////////////////////////////



	/////////////////////////////////
	// PATH UTILITIES
	// ResolvePath - resolves a relative path against the base path, or returns absolute paths unchanged
	std::filesystem::path ResolvePath(const std::string& path) const;
	/////////////////////////////////
	// SetBasePath - updates the base path used for relative path resolution
	void SetBasePath(const std::filesystem::path& basePath);
	/////////////////////////////////
	// GetBasePath - returns the current base path
	std::filesystem::path GetBasePath() const;
	/////////////////////////////////



	/////////////////////////////////
	// FILE VALIDATION
	// FileExists - checks if a file exists at the given path (resolves relative paths)
	bool FileExists(const std::string& path) const;
	/////////////////////////////////



	/////////////////////////////////
	// Private implementation
private:
	/////////////////////////////////
	// Member variables
	std::filesystem::path m_basePath;
	std::unordered_map<std::string, TileMap> m_tileMapCache; // Cache for loaded tilemaps
	/////////////////////////////////
};
/////////////////////////////////
