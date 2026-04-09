// CTileMap.h - Component wrapping TileMap data
#pragma once

#include "Component.h"
#include "TileMap.h"

// CTileMap component - stores a TileMap inside an entity so systems can operate on tilemaps via ECS
class CTileMap : public Component
{
public:
    TileMap map; // underlying tile data
    bool m_processed = false; // whether the map has been converted to collider entities
    bool m_dirty = true; // whether the map needs processing (set true when map is created or modified)

    CTileMap() = default;
    explicit CTileMap(const TileMap& m) : map(m) {}
    explicit CTileMap(TileMap&& m) : map(std::move(m)) {}

    inline int GetWidth() const { return map.width; }
    inline int GetHeight() const { return map.height; }
    inline float GetTileSize() const { return map.tileSize; }

    inline int GetTile(int x, int y) const { return map.GetTile(x, y); }
    inline void SetTile(int x, int y, int v) { map.SetTile(x, y, v); }
    inline bool IsSolid(int x, int y) const { return map.IsSolid(x, y); }
};
