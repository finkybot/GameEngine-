/////////////////////////////////
// TextureManager.cpp
/////////////////////////////////



/////////////////////////////////
// Includes and namespace aliases for the TextureManager implementation. We include necessary headers for file I/O, memory management, and SFML graphics.
#include "TextureManager.h"
#include <iostream>
/////////////////////////////////



/////////////////////////////////
// LoadAtlas - Loads a texture atlas from the specified file path and stores it under the given key. The atlas is sliced into tiles of the specified width and height. 
// If loading fails, an error message is logged and false is returned. If loading succeeds, any existing atlas for the key is replaced with the new one, and true is returned.
bool TextureManager::LoadAtlas(const std::string& key, const std::string& filePath, int tileW, int tileH) {
	// Always attempt to load the atlas from file. If loading succeeds, replace any existing atlas for the key.
	auto atlas = std::make_shared<TextureAtlas>();
	if (!atlas->LoadFromFile(filePath, tileW, tileH)) {
		std::cerr << "TextureManager: failed to load atlas '" << filePath << "' for key='" << key << "'\n";
		return false;
	}
	m_atlases[key] = atlas; // overwrite or insert
	return true;
}
/////////////////////////////////



/////////////////////////////////
// GetAtlas - Retrieves the texture atlas associated with the given key. If no atlas is found for the key, std::nullopt is returned. Otherwise, a shared pointer to the atlas is returned.
std::optional<std::shared_ptr<TextureAtlas>> TextureManager::GetAtlas(const std::string& key) const {
	auto it = m_atlases.find(key);
	if (it == m_atlases.end()) return std::nullopt;
	return it->second;
}
/////////////////////////////////



/////////////////////////////////
// UnloadAtlas - Unloads the texture atlas associated with the given key by removing it from the internal map. If no atlas exists for the key, this function does nothing.
void TextureManager::UnloadAtlas(const std::string& key) {
	m_atlases.erase(key);
}

std::vector<std::string> TextureManager::GetAtlasKeys() const {
	std::vector<std::string> keys;
	for (const auto &kv : m_atlases) keys.push_back(kv.first);
	return keys;
}
/////////////////////////////////