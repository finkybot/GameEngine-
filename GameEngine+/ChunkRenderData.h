/////////////////////////////////
// ChunkRenderData.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <vector>
/////////////////////////////////



/////////////////////////////////
//	|	ChunkRenderData struct is used to pass rendering information for a chunk to the rendering system. It contains the chunk's coordinates, dimensions, tile size, number of layers, and a pointer to the tile data for each layer. It also includes optional texture atlas information and a flag
//	|	indicating whether the chunk needs to be re-rendered.
//	|_______________________________________________________________________
struct ChunkRenderData {
	int chunkX;
	int chunkY;

	int width;
	int height;
	float tileSize;

	int numLayers;
	const std::vector<std::vector<int>>* tilesPerLayer;

	bool renderDirty = true;
};
/////////////////////////////////