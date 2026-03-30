// Raycast.h - DDA raycasting for tilemaps and ray vs AABB utilities
#pragma once

#include "Vec2.h"
#include <vector>
#include <cfloat>
#include <cmath>

struct TileMap
{
    int width = 0;
    int height = 0;
    float tileSize = 32.0f;
    std::vector<int> tiles; // 0 = empty, non-zero = solid

    TileMap() = default;
    TileMap(int w, int h, float sz = 32.0f) : width(w), height(h), tileSize(sz), tiles(w*h, 0) {}

    inline bool InBounds(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }
    inline int GetTile(int x, int y) const { return InBounds(x,y) ? tiles[y*width + x] : 0; }
    inline void SetTile(int x, int y, int v) { if (InBounds(x,y)) tiles[y*width + x] = v; }
    inline bool IsSolid(int x, int y) const { return GetTile(x,y) != 0; }
};

struct RaycastHit
{
    bool hit = false;
    int tileX = -1;
    int tileY = -1;
    Vec2 position{0,0}; // world space hit position
    Vec2 normal{0,0}; // surface normal
    float distance = 0.0f; // distance along ray (world units)
    int tileValue = 0; // value of tile hit
};

// DDA raycast against a TileMap. origin and dir should be in world coordinates; dir must be normalized.
// maxDistance in world units.
inline RaycastHit RaycastTilemapDDA(const Vec2& origin, const Vec2& dir, const TileMap& map, float maxDistance)
{
    RaycastHit result;
    if (map.width <= 0 || map.height <= 0) return result;
    if ((dir.x == 0.0f && dir.y == 0.0f)) return result;

    // Convert to grid space (cells = 1 unit)
    float invTile = 1.0f / map.tileSize;
    Vec2 originG(origin.x * invTile, origin.y * invTile);
    Vec2 dirG(dir.x * invTile, dir.y * invTile); // per-cell direction

    int mapX = static_cast<int>(std::floor(originG.x));
    int mapY = static_cast<int>(std::floor(originG.y));

    int stepX = (dirG.x < 0) ? -1 : 1;
    int stepY = (dirG.y < 0) ? -1 : 1;

    const float EPS = 1e-9f;

    float deltaDistX = (std::fabs(dirG.x) > EPS) ? std::fabs(1.0f / dirG.x) : FLT_MAX;
    float deltaDistY = (std::fabs(dirG.y) > EPS) ? std::fabs(1.0f / dirG.y) : FLT_MAX;

    float sideDistX;
    float sideDistY;

    if (dirG.x >= 0)
        sideDistX = (std::floor(originG.x) + 1.0f - originG.x) * deltaDistX;
    else
        sideDistX = (originG.x - std::floor(originG.x)) * deltaDistX;

    if (dirG.y >= 0)
        sideDistY = (std::floor(originG.y) + 1.0f - originG.y) * deltaDistY;
    else
        sideDistY = (originG.y - std::floor(originG.y)) * deltaDistY;

    float maxDistGrid = maxDistance * invTile; // max distance in grid units

    int side = 0; // 0 = hit vertical (x), 1 = hit horizontal (y)

    // Check starting cell if already inside a solid
    if (map.IsSolid(mapX, mapY))
    {
        result.hit = true;
        result.tileX = mapX;
        result.tileY = mapY;
        result.tileValue = map.GetTile(mapX, mapY);
        result.position = origin;
        result.distance = 0.0f;
        return result;
    }

    float traveled = 0.0f;
    while (traveled <= maxDistGrid)
    {
        if (sideDistX < sideDistY)
        {
            side = 0;
            mapX += stepX;
            traveled = sideDistX;
            sideDistX += deltaDistX;
        }
        else
        {
            side = 1;
            mapY += stepY;
            traveled = sideDistY;
            sideDistY += deltaDistY;
        }

        if (!map.InBounds(mapX, mapY)) break;

        if (map.IsSolid(mapX, mapY))
        {
            // hit!
            result.hit = true;
            result.tileX = mapX;
            result.tileY = mapY;
            result.tileValue = map.GetTile(mapX, mapY);

            float hitDistGrid = traveled; // grid units
            float hitDistWorld = hitDistGrid * map.tileSize; // world units
            result.distance = hitDistWorld;
            result.position = Vec2(origin.x + dir.x * hitDistWorld, origin.y + dir.y * hitDistWorld);

            // normal depending on side
            if (side == 0)
            {
                // hit vertical wall - normal points +/- x
                result.normal = Vec2((stepX > 0) ? -1.0f : 1.0f, 0.0f);
            }
            else
            {
                result.normal = Vec2(0.0f, (stepY > 0) ? -1.0f : 1.0f);
            }

            return result;
        }
    }

    // no hit
    return result;
}

// Ray vs AABB using slab method. rectMin and rectMax are world coordinates of AABB.
// dir must be normalized. Returns true and fills out t if hit at positive t <= maxDistance.
inline bool RayIntersectsAABB(const Vec2& origin, const Vec2& dir, const Vec2& rectMin, const Vec2& rectMax, float& outT, float maxDistance = FLT_MAX)
{
    const float EPS = 1e-9f;
    float tmin = 0.0f;
    float tmax = maxDistance;

    // X slab
    if (std::fabs(dir.x) < EPS)
    {
        if (origin.x < rectMin.x || origin.x > rectMax.x) return false;
    }
    else
    {
        float ood = 1.0f / dir.x;
        float t1 = (rectMin.x - origin.x) * ood;
        float t2 = (rectMax.x - origin.x) * ood;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        if (tmin > tmax) return false;
    }

    // Y slab
    if (std::fabs(dir.y) < EPS)
    {
        if (origin.y < rectMin.y || origin.y > rectMax.y) return false;
    }
    else
    {
        float ood = 1.0f / dir.y;
        float t1 = (rectMin.y - origin.y) * ood;
        float t2 = (rectMax.y - origin.y) * ood;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        if (tmin > tmax) return false;
    }

    // if we reach here, we have intersection at tmin
    if (tmin < 0.0f)
    {
        // intersection is behind origin, but may still intersect if inside the box
        if (tmax < 0.0f) return false;
        outT = tmax;
    }
    else
    {
        outT = tmin;
    }

    if (outT > maxDistance) return false;
    return true;
}
