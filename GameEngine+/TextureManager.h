/////////////////////////////////
// TextureManager.h
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations for the TextureManager class. We include necessary headers for string manipulation, memory management, and optional values.
#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <optional>

#include "TextureAtlas.h"
/////////////////////////////////



/////////////////////////////////
// TextureManager class - manages loading, retrieval, and unloading of texture atlases used for rendering tiles and sprites in the game.
class TextureManager {
	/////////////////////////////////
	// Public interface for managing texture atlases, including loading from file, retrieval by key, unloading, and listing loaded atlas keys.
public:
	/////////////////////////////////
	// Constructor and destructor for the TextureManager class. The default constructor initializes an empty manager, and the destructor cleans up any loaded atlases.
	TextureManager() = default;
	~TextureManager() = default;
	/////////////////////////////////



	/////////////////////////////////
	// Load an atlas from file and store under key. Returns true on success.
	bool LoadAtlas(const std::string& key, const std::string& filePath, int tileW, int tileH);
	/////////////////////////////////
	

	
	/////////////////////////////////
	// Get atlas by key
	std::optional<std::shared_ptr<TextureAtlas>> GetAtlas(const std::string& key) const;
	/////////////////////////////////
	 
	

	/////////////////////////////////
	// Unload atlas
	void UnloadAtlas(const std::string& key);
	/////////////////////////////////



	/////////////////////////////////
	// Get list of loaded atlas keys
	std::vector<std::string> GetAtlasKeys() const;
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for managing the loaded texture atlases. We use an unordered_map to store shared pointers to TextureAtlas objects, keyed by a string identifier.
private:
	std::unordered_map<std::string, std::shared_ptr<TextureAtlas>> m_atlases;
	/////////////////////////////////
};
/////////////////////////////////
