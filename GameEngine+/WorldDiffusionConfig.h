/////////////////////////////////
// WorldDiffusionConfig.h
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations for the WorldDiffusionConfig structure.
#pragma once
/////////////////////////////////



/////////////////////////////////
// WorldDiffusionConfig - Configuration parameters for world diffusion behavior
//								|
//								|_______________________________________________________________________
struct WorldDiffusionConfig {
	/////////////////////////////////
	float worldWidth;
	float worldHeight;

	float maxProximityDistance;	   // scales with world size
	float particleInfluenceRadius; // scales with world size
	float baseDiffusionRate;	   // scales with world size + density
	float diffusionInterval;	   // scales with world size
	float densityFactor;		   // civCount / (worldWidth * worldHeight)
	float proximityFalloffScale;   // optional tuning
	/////////////////////////////////
};
/////////////////////////////////