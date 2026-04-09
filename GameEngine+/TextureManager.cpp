#include "TextureManager.h"
#include <iostream>

bool TextureManager::LoadAtlas(const std::string& key, const std::string& filePath, int tileW, int tileH)
{
    if (m_atlases.find(key) != m_atlases.end()) return true; // already loaded
    auto atlas = std::make_shared<TextureAtlas>();
    if (!atlas->LoadFromFile(filePath, tileW, tileH))
    {
        std::cerr << "TextureManager: failed to load atlas '" << filePath << "' for key='" << key << "'\n";
        return false;
    }
    m_atlases[key] = atlas;
    return true;
}

std::optional<std::shared_ptr<TextureAtlas>> TextureManager::GetAtlas(const std::string& key) const
{
    auto it = m_atlases.find(key);
    if (it == m_atlases.end()) return std::nullopt;
    return it->second;
}

void TextureManager::UnloadAtlas(const std::string& key)
{
    m_atlases.erase(key);
}
