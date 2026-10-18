/////////////////////////////////
// ChunkRenderData.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <vector>
/////////////////////////////////



/////////////////////////////////
class TextureAtlas;
/////////////////////////////////



/////////////////////////////////
//	|	ChunkRenderData struct is used to pass rendering information for a chunk to the rendering system. It contains the chunk's coordinates, dimensions, tile size, number of layers, and a snapshot of tile data for each layer. It also includes a flag indicating whether the chunk needs to be re-rendered.
//	|_______________________________________________________________________
struct ChunkRenderData {
	int chunkX;
	int chunkY;

	int width;
	int height;
	float tileSize;

	float worldOriginX = 0.f;
	float worldOriginY = 0.f;

	// NEW: atlas pointer for GL
	TextureAtlas* atlas = nullptr;

	int numLayers;
	std::vector<std::vector<int>> tilesPerLayer;

	int activeLayer = -1;
	float unselectedLayerAlpha = 1.0f;

	bool renderDirty = true;
};
/////////////////////////////////