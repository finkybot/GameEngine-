#include "TextureAtlas.h"
#include <SFML/Graphics/Image.hpp>
#include <iostream>

bool TextureAtlas::LoadFromFile(const std::string& filePath, int tileW, int tileH)
{
    if (tileW <= 0 || tileH <= 0) return false;

    sf::Image img;
    if (!img.loadFromFile(filePath))
    {
        std::cerr << "TextureAtlas: failed to load image '" << filePath << "'\n";
        return false;
    }

    m_texture = std::make_shared<sf::Texture>();
    if (!m_texture->loadFromImage(img))
    {
        std::cerr << "TextureAtlas: failed to create texture from image '" << filePath << "'\n";
        m_texture.reset();
        return false;
    }

    m_tileW = tileW;
    m_tileH = tileH;
    m_rects.clear();

    unsigned int cols = img.getSize().x / static_cast<unsigned int>(tileW);
    unsigned int rows = img.getSize().y / static_cast<unsigned int>(tileH);
    for (unsigned int r = 0; r < rows; ++r)
    {
        for (unsigned int c = 0; c < cols; ++c)
        {
            TileRect rect;
            rect.x = static_cast<int>(c * tileW);
            rect.y = static_cast<int>(r * tileH);
            rect.w = tileW;
            rect.h = tileH;
            m_rects.push_back(rect);
        }
    }

    return !m_rects.empty();
}

std::optional<TextureAtlas::TileRect> TextureAtlas::GetRectForTile(size_t index) const
{
    if (index >= m_rects.size()) return std::nullopt;
    return m_rects[index];
}


std::optional<sf::FloatRect> TextureAtlas::GetSfFloatRectForTile(size_t index) const
{
    if (index >= m_rects.size()) return std::nullopt;
    const TileRect& t = m_rects[index];
    sf::FloatRect r;
    r.position.x = static_cast<float>(t.x);
    r.position.y = static_cast<float>(t.y);
    r.size.x = static_cast<float>(t.w);
    r.size.y = static_cast<float>(t.h);
    return r;
}
