#include "TextureManager.h"
#include <iostream>

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

std::optional<std::shared_ptr<TextureAtlas>> TextureManager::GetAtlas(const std::string& key) const {
	auto it = m_atlases.find(key);
	if (it == m_atlases.end()) return std::nullopt;
	return it->second;
}

void TextureManager::UnloadAtlas(const std::string& key) {
	m_atlases.erase(key);
}

std::vector<std::string> TextureManager::GetAtlasKeys() const {
	std::vector<std::string> keys;
	for (const auto &kv : m_atlases) keys.push_back(kv.first);
	return keys;
}
