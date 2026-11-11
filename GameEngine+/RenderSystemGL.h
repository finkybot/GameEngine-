/////////////////////////////////
// RenderSystemGL.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <glad/glad.h>
#include <SFML/Graphics.hpp>
#include <vector>
#include "Fontsystem.h"
#include "GPUInstanceStructs.h"
#include "CCamera.h"
/////////////////////////////////



/////////////////////////////////
// Forward Declarations
class EntityManager;
class CText;
class CTransform;
class TextureManager;
struct TileRenderSoA;
/////////////////////////////////



/////////////////////////////////
//	|	GPUSpriteInstance - Represents a sprite instance for GPU rendering, including position, size, rotation, texture index, and color. This struct is used for instanced rendering of sprites in OpenGL, allowing for efficient rendering of multiple sprites with varying properties.
//	|_______________________________________________________________________
struct GPUSpriteInstance {
	float x, y;			// Position
	float w, h;			// Size
	float rotation;		// Rotation
	float texIndex;		// Index into GPU texture array
	float r, g, b, a;	// Color
};
/////////////////////////////////



/////////////////////////////////
//	|	GPUCircleInstance - Represents a circle instance for GPU rendering, including position, radius, and color. This struct is used for instanced rendering of circles in OpenGL, allowing for efficient rendering of multiple circles with varying properties.
//	|_______________________________________________________________________
struct GPUCircleInstance {
	float x, y;		  // Position
	float radius;	  // Radius
	float r, g, b, a; // Color
};
/////////////////////////////////



/////////////////////////////////
//	|	GPUTextInstance - Represents a text instance for GPU rendering, including position, scale, and color. This struct is used for instanced rendering of text in OpenGL, allowing for efficient rendering of multiple text instances with varying properties.
//	|_______________________________________________________________________
struct GPUTextInstance {
	float x, y;		  // Position
	float scale;	  // Scale
	float r, g, b, a; // Color
};
/////////////////////////////////



/////////////////////////////////
//	|	GPUTextGlyphInstance - Represents a single glyph instance for GPU text rendering, including position, size, UV coordinates, and color. This struct is used for instanced rendering of individual glyphs in OpenGL, allowing for efficient rendering of text with varying properties.
//	|_______________________________________________________________________
struct GPUTextGlyphInstance {
	float texID;		// texture ID for the font atlas
	float x, y;			// glyph position in pixels
	float w, h;			// glyph size in pixels

	float u0, v0;		// UV top-left
	float u1, v1;		// UV bottom-right

	float r, g, b, a;	// color
};
/////////////////////////////////



/////////////////////////////////
//	|	RenderSystemGL - Dedicated OpenGL rendering subsystem for the game engine. This class manages the rendering of various graphical elements, including sprites, circles, and text, using GPU instancing for efficient rendering. It provides methods for 
//	|	initializing OpenGL resources, rendering instances, and handling viewport resizing.
//	|_______________________________________________________________________
class RenderSystemGL {
public:
	using TileCallback = void (*)(const EntityManager& entityManager, RenderSystemGL& renderSystem); // Callback function type for tile rendering, taking an EntityManager reference and a RenderSystemGL reference as parameters.
	void SetTileCallback(TileCallback callback) { m_tileCallback = callback; }	// Set the tile rendering callback function.

	RenderSystemGL();
	~RenderSystemGL();

	void Initialise();
	void Shutdown();

	void Render(const EntityManager& entityManager);
	void Render(const EntityManager& entityManager, const CCamera& camera);
	void OnResize(int width, int height);
	bool IsInitialized() const { return m_initialised; }
	bool IsVisible(const CCamera& camera, const Vec2& position, float halfWidth = 0.0f, float halfHeight = 0.0f) const;

	void RenderShapes(const std::vector<GPUShapeInstance>& instances, const CCamera& camera);
	void RenderGpuCircles(const std::vector<GPUShapeInstance>& instances, const CCamera& camera);
	void RenderExplosions(const std::vector<GPUInstanceData>& instances, const CCamera& camera);
	void RenderEqualizerBars(const std::vector<GPUBarInstanceData>& instances);

	void SetFontSystem(Fontsystem* fontSystem) { m_fontSystem = fontSystem; }
	void SetTextureManager(TextureManager* texManager) { m_textureManager = texManager; }
	void GetTextureManager(TextureManager*& texManager) { texManager = m_textureManager; }
	void RenderTilesInstance(TileRenderSoA& tileRenderSoA, const sf::View& view, const sf::Vector2u& windowSize);


private:
	TileCallback m_tileCallback = nullptr;

	// OpenGL resource handles
	GLuint m_quadVAO = 0;
	GLuint m_circleVAO = 0;
	GLuint m_textVAO = 0;

	// Shared quad vertex buffer object (VBO) for instanced rendering
	GLuint m_quadVBO = 0;


	// Instance buffers for different types of renderable objects
	GLuint m_spriteInstanceVBO	=	0;
	GLuint m_circleInstanceVBO	=	0;
	GLuint m_textInstanceVBO	=	0;


	// Shader program handles for different types of renderable objects
	GLuint m_spriteShaderProgram = 0;
	GLuint m_circleShaderProgram = 0;
	GLuint m_textShaderProgram = 0;

	// White texture for sprite rendering
	GLuint m_whiteTexture = 0;


	// Uniform locations for shader programs
	GLint m_spriteViewportUniformLocation = -1;
	GLint m_circleViewportUniformLocation = -1;
	GLint m_textViewportUniformLocation = -1;


	// Buffer capacities for instance buffers
	std::size_t m_spriteBufferCapacity = 0;
	std::size_t m_circleBufferCapacity = 0;
	std::size_t m_textBufferCapacity = 0;


	// Cached viewport dimensions
	int m_viewportWidth = 0;
	int m_viewportHeight = 0;
	bool m_initialised = false;

	// Text glyph GPU data
	GLuint m_textGlyphVAO = 0;
	GLuint m_textGlyphVBO = 0;
	std::vector<GPUTextGlyphInstance> m_textGlyphInstances;

	// Text shader
	GLuint m_textShader = 0;

	// --- Tile rendering resources ---
	GLuint m_tileVAO = 0;
	GLuint m_tileQuadVBO = 0;
	GLuint m_tileInstanceVBO = 0;

	GLuint m_tileShaderProgram = 0;
	GLint m_tileViewportUniformLocation = -1;

	std::size_t m_tileBufferCapacity = 0;
	TextureManager* m_textureManager = nullptr; // Pointer to the TextureManager for accessing texture atlases


	// Internal helper methods for shader compilation, buffer creation, and viewport preparation
	void CreateQuadGeometry();

	void CreateSpriteResources();
	void CreateCircleResources();
	void CreateTextResources();

	void CreateTileResources();
	void EnsureTileBufferCapacity(std::size_t requiredInstances);


	void EnsureSpriteBufferCapacity(std::size_t requiredInstances);
	void EnsureCircleBufferCapacity(std::size_t requiredInstances);
	void EnsureTextBufferCapacity(std::size_t requiredInstances);

	std::vector<GPUTextGlyphInstance> BuildGlyphInstances(const CText& CText, const CTransform& CTransform, const FontAsset& font);
	void UploadGlyphInstances();
	void RenderTextGlyphs();
	void CreateTextGlyphResources();
	void LoadTextShader();
	void LoadTileShader();

	GLuint CreateGpuShaderProgram(const char* vertexSrc, const char* fragmentSrc);
	void CreateGpuEffectBuffers();
	void CreateGpuEffectQuadGeometry();
	void CreateExplosionResources();
	void CreateEqualizerResources();
	void CreateGpuCircleResources();
	bool PrepareGpuViewport(int viewport[4]) const;
	void EnsureExplosionBufferCapacity(std::size_t requiredInstances);
	void EnsureEqualizerBufferCapacity(std::size_t requiredInstances);
	void EnsureGpuCircleBufferCapacity(std::size_t requiredInstances);


	void RenderSprites(const std::vector<GPUSpriteInstance>& instances);
	void RenderCircles(const std::vector<GPUCircleInstance>& instances);
	void RenderText(const std::vector<GPUTextInstance>& instances);
	void RenderTiles(const EntityManager& entityManager);


	GLuint CreateShaderProgram(const char* vertexSrc, const char* fragmentSrc);
	GLuint CreateShaderProgramFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
	struct Frustum2D {
		float left = 0.0f;
		float right = 0.0f;
		float top = 0.0f;
		float bottom = 0.0f;
	};
	Frustum2D BuildFrustum(const CCamera& camera) const;

	Fontsystem* m_fontSystem = nullptr; // Pointer to the FontSystem for accessing font assets

	GLuint m_gpuQuadVBO = 0;
	GLuint m_gpuCircleVAO = 0;
	GLuint m_gpuCircleInstanceVBO = 0;
	GLuint m_gpuCircleShaderProgram = 0;
	GLint m_gpuCircleViewportUniformLocation = -1;
	GLuint m_explosionVAO = 0;
	GLuint m_explosionInstanceVBO = 0;
	GLuint m_explosionShaderProgram = 0;
	GLint m_explosionViewportUniformLocation = -1;
	GLuint m_equalizerVAO = 0;
	GLuint m_equalizerInstanceVBO = 0;
	GLuint m_equalizerShaderProgram = 0;
	GLint m_equalizerViewportUniformLocation = -1;
	std::size_t m_gpuCircleBufferCapacity = 0;
	std::size_t m_explosionBufferCapacity = 0;
	std::size_t m_equalizerBufferCapacity = 0;
};
/////////////////////////////////