/////////////////////////////////
// GPUInstanceStructs.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
/////////////////////////////////


// ------------------------------------------------------------
// Per-instance data layout for GPU instancing
// ------------------------------------------------------------


////////////////////////////////
//	|	GPUInstanceData struct - Represents the per-instance data for GPU instancing of explosion effects. Each instance contains information about its position, size, age, lifetime, and color. This data is used by the GPU to render multiple instances of explosions efficiently in a single draw call.
//	|_______________________________________________________________________
struct GPUInstanceData {
	float x;		  // center X
	float y;		  // center Y
	float radius;	  // explosion radius
	float age;		  // current age
	float lifetime;	  // total lifetime
	float r, g, b, a; // color (RGBA)
};
////////////////////////////////



////////////////////////////////
//	|	GPUBarInstanceData struct - Represents the per-instance data for GPU instancing of equalizer bars. Each instance contains information about its position, size, and color. This data is used by the GPU to render multiple instances of equalizer bars efficiently in a single draw call.
//	|_______________________________________________________________________
struct GPUBarInstanceData {
	float x;
	float y;
	float halfWidth;
	float halfHeight;
	float r, g, b, a;
};
////////////////////////////////



////////////////////////////////
//	|	GPUTileInstance struct - Represents a tile instance for GPU rendering, including UV coordinates, color tint, world-space position, size, and a handle for bindless texture access. This struct is used for instanced rendering of tiles in OpenGL, allowing for efficient rendering of multiple tiles with varying properties.
//	|_______________________________________________________________________
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
////////////////////////////////



////////////////////////////////
// |	GPUTextGlyphInstance struct - Represents a text glyph instance for GPU rendering, including position, size, UV coordinates, and color. This struct is used for instanced rendering of text glyphs in OpenGL, allowing for efficient rendering of multiple glyphs with varying properties.
// |_______________________________________________________________________
struct GPUShapeInstance {
	float x;
	float y;
	float radius;
	float r;
	float g;
	float b;
	float a;
};
////////////////////////////////