/////////////////////////////////
// GPUTileInstance.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
/////////////////////////////////


/////////////////////////////////
// GPUTileInstance struct - Represents a tile instance for GPU rendering, including UV coordinates, color tint, world-space position, size, and a handle for bindless texture access. This struct is used for instanced rendering of tiles in OpenGL, allowing for efficient rendering of multiple tiles with varying properties.
//								|
//								|_______________________________________________________________________
struct GPUTileInstance {
	// --- UV rectangle ---
	float u0;
	float v0;
	float u1;
	float v1;

	// --- color tint ---
	float r;
	float g;
	float b;
	float a;

	// --- world-space position ---
	float x;
	float y;

	// --- size ---
	float w;
	float h;

    std::uint32_t handleLo;
	std::uint32_t handleHi;
};
/////////////////////////////////