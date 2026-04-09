#pragma once

#include <vector>
#include <string>
#include <optional>

// TileMap - logical representation of a 2D tile map used for raycasting, collision and rendering metadata.
struct TileMap
{
    int width = 0;
    int height = 0;
    float tileSize = 32.0f;
    std::vector<int> tiles; // 0 = empty, non-zero = solid

    // Tileset metadata (optional)
    std::string tilesetKey;     // logical key to lookup atlas in TextureManager
    std::string tilesetImage;   // optional image path (for export)
    int tilesetTileW = 0;
    int tilesetTileH = 0;

    // Optional layers support. If empty, 'tiles' is the single layer saved/loaded for compatibility.
    struct Layer { std::string name; std::vector<int> tiles; };
    std::vector<Layer> layers;

    TileMap() = default;
    TileMap(int w, int h, float sz = 32.0f) : width(w), height(h), tileSize(sz), tiles(w*h, 0) {}

    inline bool InBounds(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }
    inline int GetTile(int x, int y) const { return InBounds(x,y) ? tiles[y*width + x] : 0; }
    inline void SetTile(int x, int y, int v) { if (InBounds(x,y)) tiles[y*width + x] = v; }
    inline bool IsSolid(int x, int y) const { return GetTile(x,y) != 0; }
    // JSON serialization helpers
    bool SaveToJSON(const std::string& path, std::string* outErr = nullptr) const;
    static std::optional<TileMap> LoadFromJSON(const std::string& path, std::string* outErr = nullptr);
    // Legacy helpers kept for compatibility (forward to the TileMap methods)
    bool SaveToJSON_Legacy(const std::string& path, std::string* outErr = nullptr) const { return SaveToJSON(path, outErr); }
    static std::optional<TileMap> LoadFromJSON_Legacy(const std::string& path, std::string* outErr = nullptr) { return LoadFromJSON(path, outErr); }
};
