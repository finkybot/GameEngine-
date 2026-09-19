/////////////////////////////////
// GPUTileInstance.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
/////////////////////////////////



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
};
