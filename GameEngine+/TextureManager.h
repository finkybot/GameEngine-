#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <optional>

#include "TextureAtlas.h"

class TextureManager {
public:
	TextureManager() = default;
	~TextureManager() = default;

	// Load an atlas from file and store under key. Returns true on success.
	bool LoadAtlas(const std::string& key, const std::string& filePath, int tileW, int tileH);

	// Get atlas by key
	std::optional<std::shared_ptr<TextureAtlas>> GetAtlas(const std::string& key) const;

	// Unload atlas
	void UnloadAtlas(const std::string& key);

private:
	std::unordered_map<std::string, std::shared_ptr<TextureAtlas>> m_atlases;
};
