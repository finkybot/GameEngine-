/////////////////////////////////
// GPURenderSystem.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once

#include <glad/glad.h>
#include <SFML/OpenGL.hpp>
#include <vector>
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



// ------------------------------------------------------------
// GPURenderSystem - Dedicated OpenGL rendering subsystem
// ------------------------------------------------------------


////////////////////////////////
//	|	GPURenderSystem class - A dedicated OpenGL rendering subsystem for rendering GPU-driven effects such as explosions and equalizer bars. This class manages OpenGL resources, shader programs, and buffers for efficient rendering of multiple instances of effects in a single draw call.
//	|	It provides methods for initializing, rendering, and managing the viewport.
class GPURenderSystem {
public:
	GPURenderSystem();
	~GPURenderSystem();

	// Must be called once after GLAD is initialized
	void Initialize();

	// Release GL resources while a valid context is active
	void Shutdown();

	bool IsInitialized() const { return m_initialized; }

	// Called every frame to render GPU-driven effects
	void RenderExplosions(const std::vector<GPUInstanceData>& instances);
	void RenderEqualizerBars(const std::vector<GPUBarInstanceData>& instances);

	// Optional: resize viewport if window size changes
	void OnResize(int width, int height);

private:
	// Internal helpers
	GLuint CreateShaderProgram(const char* vertexSrc, const char* fragmentSrc);
	void CreateBuffers();
	void CreateQuadGeometry();
	void CreateExplosionResources();
	void CreateEqualizerResources();
	bool PrepareViewport(int viewport[4]) const;
	void EnsureExplosionBufferCapacity(std::size_t requiredInstances);
	void EnsureEqualizerBufferCapacity(std::size_t requiredInstances);

private:
	// GL objects
	GLuint m_quadVBO = 0;
	GLuint m_explosionVAO = 0;
	GLuint m_explosionInstanceVBO = 0;
	GLuint m_explosionShaderProgram = 0;
	GLint m_explosionViewportUniformLocation = -1;
	GLuint m_equalizerVAO = 0;
	GLuint m_equalizerInstanceVBO = 0;
	GLuint m_equalizerShaderProgram = 0;
	GLint m_equalizerViewportUniformLocation = -1;
	std::size_t m_explosionBufferCapacity = 0;
	std::size_t m_equalizerBufferCapacity = 0;

	// Cached viewport size
	int m_viewportWidth = 0;
	int m_viewportHeight = 0;

	bool m_initialized = false;
};
////////////////////////////////