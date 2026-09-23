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
//	|	TextureManager class - manages loading, retrieval, and unloading of texture atlases used for rendering tiles and sprites in the game.
//	|_______________________________________________________________________
class TextureManager {
public:
	// Constructor and destructor for the TextureManager class. The default constructor initializes an empty manager, and the destructor cleans up any loaded atlases.
	TextureManager() = default;
	~TextureManager() = default;

	// ---------------------------------------------------------
	// SFML API
	// ---------------------------------------------------------

	bool LoadAtlas(const std::string& key, const std::string& filePath, int tileW, int tileH);	// Load an atlas from file and store under key. Returns true on success.
	std::optional<std::shared_ptr<TextureAtlas>> GetAtlas(const std::string& key) const;		// Get atlas by key
	
	void UnloadAtlas(const std::string& key);													// Unload atlas
	
	std::vector<std::string> GetAtlasKeys() const;												// Get list of loaded atlas key

	
    // ---------------------------------------------------------
	// OpenGL API
	// ---------------------------------------------------------

	bool LoadAtlasGL(const std::string& key);												// Load GL texture and build GL rects for an already loaded atlas.
	std::optional<std::shared_ptr<TextureAtlas>> GetAtlasGL(const std::string& key) const;	// Get atlas by key (GL)



private:
	std::unordered_map<std::string, std::shared_ptr<TextureAtlas>> m_atlases;
};
/////////////////////////////////
