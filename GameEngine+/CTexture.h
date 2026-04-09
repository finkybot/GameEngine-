#pragma once
#include "Component.h"
#include <string>

// CTexture: simple data component referencing a texture atlas and a tile index
class CTexture : public Component
{
public:
    std::string atlasKey;   // key used with TextureManager
    int tileIndex = 0;      // index into the atlas (0-based)
    bool visible = true;
    float zOrder = 0.0f;
    // Area (world) covered by this texture. If zero, texture represents a single tile sized by the atlas.
    float areaW = 0.0f;
    float areaH = 0.0f;

    CTexture() = default;
    explicit CTexture(const std::string& key, int idx = 0) : atlasKey(key), tileIndex(idx) {}
    explicit CTexture(const std::string& key, int idx, float w, float h) : atlasKey(key), tileIndex(idx), areaW(w), areaH(h) {}
};
