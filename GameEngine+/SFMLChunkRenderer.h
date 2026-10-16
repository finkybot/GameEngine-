/////////////////////////////////
// SFMLChunkRenderer.h
/////////////////////////////////



/////////////////////////////////
#pragma once
#include <SFML/Graphics.hpp>
#include "ChunkRenderData.h"
/////////////////////////////////



/////////////////////////////////
//	|	SFMLChunkRenderer namespace contains functions for rendering chunks using SFML. It provides a simple interface to draw chunks based on their rendering data, allowing for easy integration with SFML's rendering system.
//	|_______________________________________________________________________
namespace SFMLChunkRenderer {

	void RenderChunk(const ChunkRenderData& data, sf::RenderWindow& window);
};
/////////////////////////////////
