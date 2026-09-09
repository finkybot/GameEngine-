/////////////////////////////////
// RenderSystemGL.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "RenderSystemGL.h"
#include "EntityManager.h"
#include "Entity.h"
#include "CTransform.h"
#include "CTexture.h"
#include "CCircle.h"
#include "CText.h"
#include <iostream>
/////////////////////////////////



/////////////////////////////////
// Quad covering (0,0) to (1,1) — scaled in shader using w/h
static const float quadVertices[] = {
	// x, y
    -1.0f, -1.0f,
     1.0f, -1.0f,
    -1.0f,  1.0f,
     1.0f,  1.0f
};
/////////////////////////////////



/////////////////////////////////
// VERTEX AND FRAGMENT SHADER SOURCES


// ---------------------------------
// VERTEX SHADER (spriteVertexSrc)
// ---------------------------------
static const char* spriteVertexSrc = R"(
#version 330 core

layout(location = 0) in vec2 aPos;          // quad vertex
layout(location = 1) in vec2 iPos;          // instance position
layout(location = 2) in vec2 iSize;         // instance size
layout(location = 3) in float iRotation;    // instance rotation
layout(location = 4) in float iTexIndex;    // texture array index
layout(location = 5) in vec4 iColor;        // tint color

uniform vec2 uViewportSize;

out vec2 vUV;
out float vTexIndex;
out vec4 vColor;

void main()
{
    // Rotate quad
    float cs = cos(radians(iRotation));
    float sn = sin(radians(iRotation));
    vec2 rotated = vec2(
        aPos.x * cs - aPos.y * sn,
        aPos.x * sn + aPos.y * cs
    );

    // Convert size from pixel space to NDC space (0.5 to -0.5 range, so iSize/2 is the half-size)
    vec2 ndcSize = vec2(
        (iSize.x / uViewportSize.x),
        (iSize.y / uViewportSize.y)
    );

    // Scale the rotated quad by the NDC size
    vec2 scaledRotated = rotated * ndcSize;

    // Convert position to NDC
    vec2 posNDC = vec2(
        (iPos.x / uViewportSize.x) * 2.0 - 1.0,
        1.0 - (iPos.y / uViewportSize.y) * 2.0
    );

    // Final position in NDC
    vec2 ndc = posNDC + scaledRotated;
    gl_Position = vec4(ndc, 0.0, 1.0);

    // UV from quad position (0..1)
    vUV = aPos * 0.5 + 0.5;

    vTexIndex = iTexIndex;
    vColor = iColor;
}
)";



// ---------------------------------
// FRAGMENT SHADER (spriteFragmentSrc)
// ---------------------------------
static const char* spriteFragmentSrc = R"(
#version 330 core

in vec2 vUV;
in float vTexIndex;
in vec4 vColor;

uniform sampler2D uTextures[32];

out vec4 fragColor;

void main()
{
   int idx = int(vTexIndex);
   vec4 texColor = texture(uTextures[idx], vUV);
   fragColor = texColor * vColor;
}
)";



// --------------------------------
// VERTEX SHADER (circleVertexSrc)
// --------------------------------
static const char* circleVertexSrc = R"(
#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 6) in vec2 iPos;
layout(location = 7) in float iRadius;
layout(location = 8) in vec4 iColor;

uniform vec2 uViewportSize;

out vec2 vLocalPos;
out vec4 vColor;

void main()
{
    vec2 posNDC = vec2(
        (iPos.x / uViewportSize.x) * 2.0 - 1.0,
        1.0 - (iPos.y / uViewportSize.y) * 2.0
    );

    vec2 ndcRadius = vec2(
        iRadius / uViewportSize.x,
        iRadius / uViewportSize.y
    );

    vec2 scaledPos = aPos * ndcRadius;

    gl_Position = vec4(posNDC + scaledPos, 0.0, 1.0);

    vLocalPos = aPos;   // unit circle space
    vColor = iColor;
}

)";



// --------------------------------
// FRAGMENT SHADER (circleFragmentSrc)
// --------------------------------
static const char* circleFragmentSrc = R"(
#version 330 core

in vec2 vLocalPos;
in vec4 vColor;

out vec4 fragColor;

void main()
{
    float dist = length(vLocalPos);
    if (dist > 1.0) discard;

    fragColor = vColor;
}
)";



// --------------------------------
// VERTEX SHADER (textVertexSrc) - Temporary
// --------------------------------
static const char* textVertexSrc = R"(
#version 330 core

layout(location = 0) in vec2 aPos;          // quad vertex (0..1)
layout(location = 9) in vec2 iPos;          // glyph position
layout(location = 10) in vec2 iSize;        // glyph size
layout(location = 11) in vec4 iUV;          // u0,v0,u1,v1
layout(location = 12) in vec4 iColor;       // glyph color

uniform vec2 uViewportSize;

out vec2 vUV;
out vec4 vColor;

void main()
{
    // Convert glyph position to NDC
    vec2 posNDC = vec2(
        (iPos.x / uViewportSize.x) * 2.0 - 1.0,
        1.0 - (iPos.y / uViewportSize.y) * 2.0
    );

    // Convert glyph size to NDC
    vec2 ndcSize = vec2(
        iSize.x / uViewportSize.x,
        iSize.y / uViewportSize.y
    );

    // Scale quad
    vec2 scaledPos = aPos * ndcSize;

    gl_Position = vec4(posNDC + scaledPos, 0.0, 1.0);

    // Interpolate UVs
	vec2 uvT = clamp(aPos, 0.0, 1.0);
    vUV = mix(iUV.xy, iUV.zw, uvT);

    vColor = iColor;
}
)";




// --------------------------------
// FRAGMENT SHADER (textFragmentSrc) - Temporary
// --------------------------------
static const char* textFragmentSrc = R"(
#version 330 core

in vec2 vUV;
in vec4 vColor;

uniform sampler2D uAtlas;

out vec4 fragColor;

void main()
{
    float alpha = texture(uAtlas, vUV).r;
    fragColor = vec4(vColor.rgb, vColor.a * alpha);
}
)";


/////////////////////////////////
// RenderSystemGL Implementation
RenderSystemGL::RenderSystemGL() {}
/////////////////////////////////



/////////////////////////////////
// Destructor
RenderSystemGL::~RenderSystemGL() {
	Shutdown();
}
/////////////////////////////////



/////////////////////////////////
// Initialise - Sets up OpenGL resources, shaders, and buffers for rendering. This method should be called once after the OpenGL context is initialized.
void RenderSystemGL::Initialise() {
	// Check if already initialised, get out if so
	if (m_initialised) return; 

	// Check if the Fontsystem pointer is set before proceeding with initialization
	if (!m_fontSystem) {
		std::cerr << "[RenderSystemGL] Fontsystem not set before Initialise()" << std::endl;
		return;
	}

	// Check for OpenGL context and required functions
	const GLubyte* version = glGetString(GL_VERSION);
	if (!version) {
		std::cerr << "[RenderSystemGL] No active OpenGL context during Initialise()" << std::endl;
		return;
	}

	// Set clear color to black
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Disable depth testing for 2D rendering
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);

	// Check for required OpenGL functions for instancing
	CreateQuadGeometry();

	CreateSpriteResources();

	CreateCircleResources();
	CreateTextResources();


	CreateTextGlyphResources(); // VAO/VBO for glyph instances
	LoadTextShader();			// shader for glyph rendering

	// Set initialised flag to true after successful setup
	m_initialised = true;
	printf("\x1b[36m[RenderSystemGL::Initialise]\x1b[0m Initialization complete\n");
}
/////////////////////////////////



/////////////////////////////////
// Shutdown - Cleans up OpenGL resources. This method should be called when the rendering system is no longer needed, typically during application shutdown.
void RenderSystemGL::Shutdown() {
	// Delete shader programs
	if (m_quadVBO) { glDeleteBuffers(1, &m_quadVBO);
		m_quadVBO = 0;
	}

	// Delete VAO
	if (m_quadVAO) {
		glDeleteVertexArrays(1, &m_quadVAO);
		m_quadVAO = 0;
	}

	// Delete instance buffers for sprites
	if (m_spriteInstanceVBO) { glDeleteBuffers(1, &m_spriteInstanceVBO);
		m_spriteInstanceVBO = 0;
	}

	// Delete instance buffers for circles
	if (m_circleInstanceVBO) { glDeleteBuffers(1, &m_circleInstanceVBO);
		m_circleInstanceVBO = 0;
	}

	// Delete instance buffers for text
	if (m_textInstanceVBO) { glDeleteBuffers(1, &m_textInstanceVBO);
		m_textInstanceVBO = 0;
	}

	// Delete shader programs for sprites
	if (m_spriteShaderProgram) {
		glDeleteProgram(m_spriteShaderProgram);
		m_spriteShaderProgram = 0;
	}

	// Delete shader programs for circles
	if (m_circleShaderProgram) {
		glDeleteProgram(m_circleShaderProgram);
		m_circleShaderProgram = 0;
	}

	// Delete shader programs for text
	if (m_textShaderProgram) {
		glDeleteProgram(m_textShaderProgram);
		m_textShaderProgram = 0;
	}

	// Delete white texture
	if (m_whiteTexture) {
		glDeleteTextures(1, &m_whiteTexture);
		m_whiteTexture = 0;
	}

	// Reset uniform locations
	m_initialised = false;
}
/////////////////////////////////



/////////////////////////////////
// Render - Main rendering function that takes an EntityManager and renders all entities based on their components. This method should be called every frame to render the current state of the game world.
void RenderSystemGL::Render(const EntityManager& entityManager) {
	// Ensure the rendering system is initialised before proceeding
	if (!m_initialised) {
		std::cerr << "[RenderSystemGL] Render called before Initialise()" << std::endl;
		return;
	}

	// *** DEBUGGING: Print the number of entities to be rendered
	//printf("[RenderSystemGL::Render] Starting render with %zu entities\n", entityManager.GetEntities().size());

	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT);
	
	// *** DEBUGGING: Print a message indicating that glClear has been called
	//printf("[RenderSystemGL::Render] Called glClear\n");

	// Prepare viewport dimensions for rendering
	std::vector<GPUSpriteInstance> spriteInstances;
	std::vector<GPUCircleInstance> circleInstances;	
	std::vector<GPUTextInstance> textInstances;

	// Iterate through all entities in the EntityManager and collect their renderable components for rendering
	for (auto& entity : entityManager.GetEntities()) {

		// Check if the entity has a CTransform component, which is required for rendering
		if (auto transform = entity->GetComponent<CTransform>()) {

			// ---------------------------------
			//  SPRITES (CTexture)
			// ---------------------------------
			// Check if the entity has a CTexture component, which is required for sprite rendering
			if (auto texture = entity->GetComponent<CTexture>()) {

				GPUSpriteInstance spriteInstance{};
				spriteInstance.x = transform->position.x;
				spriteInstance.y = transform->position.y;

				spriteInstance.w = texture->areaW > 0 ? texture->areaW : 32.0f; // Default width if areaW is not set
				spriteInstance.h = texture->areaH > 0 ? texture->areaH : 32.0f; // Default height if areaH is not set

				spriteInstance.rotation = texture->rotation;
				spriteInstance.texIndex = static_cast<float>(texture->gpuIndex);

				spriteInstance.r = texture->color.r / 255.f;
				spriteInstance.g = texture->color.g / 255.f;
				spriteInstance.b = texture->color.b / 255.f;
				spriteInstance.a = texture->color.a / 255.f;

				spriteInstances.push_back(spriteInstance);
			}


			// ---------------------------------
			// CIRCLES (CCircleGPU)
			// ---------------------------------
			// Check if the entity has a CCircleGPU component, which is required for circle rendering
			if (auto circle = entity->GetComponent<CCircleGPU>()) {
				GPUCircleInstance circleInstance{};
				
				circleInstance.x = transform->position.x;
				circleInstance.y = transform->position.y;
				
				circleInstance.radius = circle->radius;
				
				circleInstance.r = circle->color.r / 255.f;
				circleInstance.g = circle->color.g / 255.f;
				circleInstance.b = circle->color.b / 255.f;
				circleInstance.a = circle->color.a / 255.f;
				
				circleInstances.push_back(circleInstance);
			}

			// ---------------------------------
			// TEXT (CText)
			// ---------------------------------
			// Check if the entity has a CText component, which is required for text rendering
			if (auto text = entity->GetComponent<CText>()) {
				// Retrieve the font asset from the Fontsystem using the font key specified in the CText component
				const FontAsset* font = m_fontSystem->GetFont(text->fontKey);
				
				// Build glyph instances for the text using the CText component, CTransform component, and the retrieved font asset
				auto glyphs = BuildGlyphInstances(*text, *transform, *font);
				
				// Append the generated glyph instances to the main text glyph instance vector for rendering
				m_textGlyphInstances.insert(m_textGlyphInstances.end(), glyphs.begin(), glyphs.end());
			}
		}
	}

	// ---------------------------------
	// Render the collected instances for sprites, circles, and text
	// ---------------------------------
	if (!spriteInstances.empty()) {
		RenderSprites(spriteInstances);
	}

	if (!circleInstances.empty()) {
		RenderCircles(circleInstances);
	}

	// ---------------------------------
	// Render text glyphs if there are any glyph instances to render
	// ---------------------------------
	if (!m_textGlyphInstances.empty()) {
		//std::cout << "Glyph count: " << m_textGlyphInstances.size() << std::endl;
		UploadGlyphInstances();
		RenderTextGlyphs();
	}
}
/////////////////////////////////



/////////////////////////////////
// OnResize - Handles window resize events. This method can be used to update viewport dimensions or other rendering parameters when the window size changes.
void RenderSystemGL::OnResize(int width, int height) {
	// Update cached viewport dimensions
	m_viewportWidth = width;
	m_viewportHeight = height;

	// Update OpenGL viewport
	glViewport(0, 0, width, height);

	// ---------------------------------
	// Update sprite shader program uniform for viewport dimensions if needed
	// ---------------------------------
	// Update uniform locations for viewport dimensions in shader programs if needed
	if (m_spriteShaderProgram && m_spriteViewportUniformLocation >= 0) {
		// Use the sprite shader program and set the viewport uniform
		glUseProgram(m_spriteShaderProgram);
		glUniform2f(m_spriteViewportUniformLocation, static_cast<float>(width), static_cast<float>(height));
	}


	// ---------------------------------
	// Update circle shader program uniform for viewport dimensions if needed
	// ---------------------------------
	if (m_circleShaderProgram && m_circleViewportUniformLocation >= 0) {
		// Use the circle shader program and set the viewport uniform
		glUseProgram(m_circleShaderProgram);
		glUniform2f(m_circleViewportUniformLocation, static_cast<float>(width), static_cast<float>(height));
	}


	// ---------------------------------
	// Update text shader program uniform for viewport dimensions if needed
	// ---------------------------------
	if (m_textShaderProgram && m_textViewportUniformLocation >= 0) {
		// Use the text shader program and set the viewport uniform
		glUseProgram(m_textShaderProgram);
		glUniform2f(m_textViewportUniformLocation, static_cast<float>(width), static_cast<float>(height));
	}

	glUseProgram(0); // Unbind any shader program after updating uniforms
}
/////////////////////////////////



/////////////////////////////////
// Private helper methods for shader compilation, buffer creation, and viewport preparation
void RenderSystemGL::CreateQuadGeometry() {
	// Create a simple quad geometry for instanced rendering
	// Quad vertices in Normalized Device Coordinates (NDC) space (x, y) half-size quad centered at the origin. The vertex shader will scale and translate these based on instance data.
	const float quadVerts[] = {
		-1.0f, -1.0f, // Bottom-left
		-1.0f,  1.0f, // Top-left
		 1.0f, -1.0f, // Bottom-right
		 1.0f,  1.0f  // Top-right
	};

	// Create and bind the quad vertex array object (VAO)
	glGenVertexArrays(1, &m_quadVAO);
	glBindVertexArray(m_quadVAO);
	
	// Create and bind the quad vertex buffer object (VBO)
	glGenBuffers(1, &m_quadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);

	// Set up vertex attribute for quad positions (location = 0)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0); // 2 floats per vertex (x, y)

	// Unbind the VBO and VAO to avoid accidental modification
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
/////////////////////////////////



/////////////////////////////////
// CreateSpriteResources - Creates OpenGL resources (shaders, VAO, VBO) for rendering sprites.
void RenderSystemGL::CreateSpriteResources() {
	// ---------------------------------
	// 1. Compile sprite shaders (vertex and fragment) and create shader program
	// ---------------------------------
	m_spriteShaderProgram = CreateShaderProgram(spriteVertexSrc, spriteFragmentSrc);

	// Check if shader program creation was successful
	if (!m_spriteShaderProgram) {
		std::cerr << "[RenderSystemGL] Failed to create sprite shader program" << std::endl;
		return;
	}

	m_spriteViewportUniformLocation = glGetUniformLocation(m_spriteShaderProgram, "uViewportSize");

	// Bind the shader program to set up texture array uniforms
	for (int i = 0; i < 32; ++i) {
		std::string uniformName = "uTextures[" + std::to_string(i) + "]";
		GLint location = glGetUniformLocation(m_spriteShaderProgram, uniformName.c_str());
		if (location < 0) {
			std::cerr << "[RenderSystemGL] Failed to locate sprite texture uniform: " << uniformName << std::endl;
		}
	}

	// ---------------------------------
	// 2. Bind a dummy 1x1 white texture to texture unit 0
	// ---------------------------------
	glGenTextures(1, &m_whiteTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_whiteTexture);

	unsigned char pixel[4] = {255, 255, 255, 255};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glUseProgram(m_spriteShaderProgram);
	GLint tex0Loc = glGetUniformLocation(m_spriteShaderProgram, "uTextures[0]");
	if (tex0Loc >= 0) {
		glUniform1i(tex0Loc, 0); // bind sampler index 0 → GL_TEXTURE0
	}
	glUseProgram(0);

	// ---------------------------------
	// 3. Create instance buffer
	// ---------------------------------
	glGenBuffers(1, &m_spriteInstanceVBO);

	// ---------------------------------
	// 4. Create VAO for sprite rendering
	// ---------------------------------
	glBindVertexArray(m_quadVAO); // Use the shared quad VAO
	glBindBuffer(GL_ARRAY_BUFFER, m_spriteInstanceVBO);

	std::size_t stride = sizeof(GPUSpriteInstance);
	printf("\x1b[33m[CreateSpriteResources]\x1b[0m Sprite stride: %zu bytes\n", stride);

	// Position (location = 1)
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glVertexAttribDivisor(1, 1); // Advance per instance
	printf("\x1b[33m[CreateSpriteResources]\x1b[0m Set up location 1 (position)\n");

	// Size (location = 2)
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 2));
	glVertexAttribDivisor(2, 1); // Advance per instance
	printf("\x1b[33m[CreateSpriteResources]\x1b[0m Set up location 2 (size)\n");

	// Rotation (location = 3)
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 4));
	glVertexAttribDivisor(3, 1); // Advance per instance

	// Texture Index (location = 4)
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 5));
	glVertexAttribDivisor(4, 1); // Advance per instance

	// Color (location = 5)
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 6));
	glVertexAttribDivisor(5, 1); // Advance per instance

	// Unbind the VBO and VAO to avoid accidental modification
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	printf("\x1b[33m[CreateSpriteResources]\x1b[0m Sprite VAO setup complete\n");
}
/////////////////////////////////



/////////////////////////////////
// CreateCircleResources - Creates OpenGL resources (shaders, VAO, VBO) for rendering circles.
void RenderSystemGL::CreateCircleResources() {
	// ---------------------------------
	// 1. Compile circle shaders
	// ---------------------------------
	m_circleShaderProgram = CreateShaderProgram(circleVertexSrc, circleFragmentSrc);

	// Check if shader program creation was successful
	if (!m_circleShaderProgram) {
		std::cerr << "\x1b[39m[RenderSystemGL]\x1b[0m Failed to create circle shader program" << std::endl;
		return;
	}

	// Use the circle shader program to get the uniform location for viewport size
	glUseProgram(m_circleShaderProgram);

	m_circleViewportUniformLocation = glGetUniformLocation(m_circleShaderProgram, "uViewportSize");

	glUseProgram(0); // Unbind shader program after setup

	// ---------------------------------
	// 2. Create instance buffer
	// ---------------------------------
	glGenBuffers(1, &m_circleInstanceVBO);

	// ---------------------------------
	// 3. Create VAO for circle rendering
	// ---------------------------------
	glGenVertexArrays(1, &m_circleVAO);
	glBindVertexArray(m_circleVAO); // Use the circle VAO

	// ---------------------------------
	// Bind the shared quad VBO for circle rendering
	// ---------------------------------
	glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0); // 2 floats per vertex (x, y)


	glBindBuffer(GL_ARRAY_BUFFER, m_circleInstanceVBO);

	std::size_t stride = sizeof(GPUCircleInstance);

	// Position
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glVertexAttribDivisor(6, 1); // Advance per instance

	// Radius
	glEnableVertexAttribArray(7);
	glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 2));
	glVertexAttribDivisor(7, 1); // Advance per instance

	// Color
	glEnableVertexAttribArray(8);
	glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 3));
	glVertexAttribDivisor(8, 1); // Advance per instance

	// Unbind the VBO and VAO to avoid accidental modification
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
/////////////////////////////////



/////////////////////////////////
// CreateTextResources - Creates OpenGL resources (shaders, VAO, VBO) for rendering text. This method is a placeholder and does not perform any actions in this implementation.
void RenderSystemGL::CreateTextResources() {
	// ---------------------------------
	// 1. Compile text shaders
	// ---------------------------------
	m_textShaderProgram = CreateShaderProgram(textVertexSrc, textFragmentSrc);

	// Check if shader program creation was successful
	if (!m_textShaderProgram) {
		std::cerr << "\x1b[39m[RenderSystemGL]\x1b[0m Failed to create text shader program" << std::endl;
		return;
	}

	// Use the text shader program to get the uniform location for viewport size
	glUseProgram(m_textShaderProgram);
	m_textViewportUniformLocation = glGetUniformLocation(m_textShaderProgram, "uViewportSize");
	glUseProgram(0); // Unbind shader program after setup

	// ---------------------------------
	// 2. Create instance buffer
	// ---------------------------------
	glGenBuffers(1, &m_textInstanceVBO);


	// ---------------------------------
	// 3. Create VAO for text rendering
	// ---------------------------------
	glGenVertexArrays(1, &m_textVAO);
	glBindVertexArray(m_textVAO);

// --- Create a dedicated quad VBO for text (0..1 space) ---
	GLuint textQuadVBO;
	glGenBuffers(1, &textQuadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, textQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	// Attribute 0 → aPos (0..1 quad)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);

	// Bind text instance VBO (attributes 9, 10, 11)
	glBindBuffer(GL_ARRAY_BUFFER, m_textInstanceVBO);

	std::size_t stride = sizeof(GPUTextGlyphInstance);

    // ---------------------------------
	// Attribute 9 → x, y
	// ---------------------------------
	glEnableVertexAttribArray(9);
	glVertexAttribPointer(9, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GPUTextGlyphInstance, x));
	glVertexAttribDivisor(9, 1);

	// ---------------------------------
	// Attribute 10 → w, h
	// ---------------------------------
	glEnableVertexAttribArray(10);
	glVertexAttribPointer(10, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GPUTextGlyphInstance, w));
	glVertexAttribDivisor(10, 1);

	// ---------------------------------
	// Attribute 11 → u0, v0, u1, v1
	// ---------------------------------
	glEnableVertexAttribArray(11);
	glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GPUTextGlyphInstance, u0));
	glVertexAttribDivisor(11, 1);

	// ---------------------------------
	// Attribute 12 → r, g, b, a
	// ---------------------------------
	glEnableVertexAttribArray(12);
	glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(GPUTextGlyphInstance, r));
	glVertexAttribDivisor(12, 1);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
/////////////////////////////////



/////////////////////////////////
// EnsureSpriteBufferCapacity - Ensures that the sprite instance buffer has enough capacity to hold the required number of instances. If not, it reallocates the buffer with increased capacity.
void RenderSystemGL::EnsureSpriteBufferCapacity(std::size_t requiredInstances) {
	//printf("[EnsureSpriteBufferCapacity] Required: %zu, Current capacity: %zu\n", requiredInstances, m_spriteBufferCapacity);

	// If we already have enough capacity, return early
	if (requiredInstances <= m_spriteBufferCapacity) {
		
		// DEBUG: Print a message indicating that the current capacity is sufficient
		// printf("[EnsureSpriteBufferCapacity] Capacity sufficient, returning\n");
		
		return;
	}

	// Calculate new capacity (double the current capacity or set to requiredInstances if current is 0)
	std::size_t newCapacity = m_spriteBufferCapacity > 0 ? m_spriteBufferCapacity * 2 : requiredInstances;

	// Keep doubling until we have enough capacity
	while (newCapacity < requiredInstances) {
		newCapacity *= 2; // Keep doubling until we have enough capacity
	}

	// Create a new buffer with the new capacity
	m_spriteBufferCapacity = newCapacity;

	// DEBUG: Print a message indicating that a new buffer is being allocated
	// printf("[EnsureSpriteBufferCapacity] Allocating new buffer with capacity: %zu\n", m_spriteBufferCapacity);

	// Bind the VAO first to establish the buffer binding
	glBindVertexArray(m_quadVAO);

	// Allocate new buffer on GPU
	glBindBuffer(GL_ARRAY_BUFFER, m_spriteInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, m_spriteBufferCapacity * sizeof(GPUSpriteInstance), nullptr, GL_DYNAMIC_DRAW);

	// Re-setup vertex attribute pointers after buffer reallocation
	std::size_t stride = sizeof(GPUSpriteInstance);

	// Position (location = 1)
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glVertexAttribDivisor(1, 1);

	// Size (location = 2)
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 2));
	glVertexAttribDivisor(2, 1);

	// Rotation (location = 3)
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 4));
	glVertexAttribDivisor(3, 1);

	// Texture Index (location = 4)
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 5));
	glVertexAttribDivisor(4, 1);

	// Color (location = 5)
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 6));
	glVertexAttribDivisor(5, 1);

	// Unbind the buffer and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
/////////////////////////////////



/////////////////////////////////
// EnsureCircleBufferCapacity - Ensures that the circle instance buffer has enough capacity to hold the required number of instances. If not, it reallocates the buffer with increased capacity.
void RenderSystemGL::EnsureCircleBufferCapacity(std::size_t requiredInstances) {
	// If we already have enough capacity, do nothing
	if (requiredInstances <= m_circleBufferCapacity) return;

	// Calculate new capacity (double the current capacity or set to requiredInstances if current is 0)
	std::size_t newCapacity = m_circleBufferCapacity > 0 ? m_circleBufferCapacity * 2 : requiredInstances;

	// Keep doubling until we have enough capacity
	while (newCapacity < requiredInstances) {
		newCapacity *= 2; // Keep doubling until we have enough capacity
	}

	// Create a new buffer with the new capacity
	m_circleBufferCapacity = newCapacity;

	// Bind the VAO first to establish the buffer binding
	glBindVertexArray(m_quadVAO);

	// Allocate new buffer on GPU
	glBindBuffer(GL_ARRAY_BUFFER, m_circleInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, m_circleBufferCapacity * sizeof(GPUCircleInstance), nullptr, GL_DYNAMIC_DRAW);

	// Re-setup vertex attribute pointers after buffer reallocation
	std::size_t stride = sizeof(GPUCircleInstance);

	// Position
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glVertexAttribDivisor(6, 1);

	// Radius
	glEnableVertexAttribArray(7);
	glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 2));
	glVertexAttribDivisor(7, 1);

	// Color
	glEnableVertexAttribArray(8);
	glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 3));
	glVertexAttribDivisor(8, 1);

	// Unbind the buffer and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
/////////////////////////////////



/////////////////////////////////
// EnsureTextBufferCapacity - Ensures that the text instance buffer has enough capacity to hold the required number of instances. If not, it reallocates the buffer with increased capacity.
void RenderSystemGL::EnsureTextBufferCapacity(std::size_t requiredInstances) {
	// If we already have enough capacity, get out early
	if (requiredInstances <= m_textBufferCapacity)	return;

	// Calculate new capacity (double the current capacity or set to requiredInstances if current is 0)
	std::size_t newCapacity = m_textBufferCapacity > 0 ? m_textBufferCapacity * 2 : requiredInstances;

	// Keep doubling until we have enough capacity
	while (newCapacity < requiredInstances)	newCapacity *= 2;

	// Create a new buffer with the new capacity
	m_textBufferCapacity = newCapacity;

	// Bind the VAO first to establish the buffer binding
	glBindVertexArray(m_textVAO);

	// Reallocate instance buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_textInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, m_textBufferCapacity * sizeof(GPUTextInstance), nullptr, GL_DYNAMIC_DRAW);

	// Re-setup vertex attribute pointers after buffer reallocation
	std::size_t stride = sizeof(GPUTextInstance);

	// Rebind attributes
	glEnableVertexAttribArray(9);
	glVertexAttribPointer(9, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glVertexAttribDivisor(9, 1);

	glEnableVertexAttribArray(10);
	glVertexAttribPointer(10, 1, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 2));
	glVertexAttribDivisor(10, 1);

	glEnableVertexAttribArray(11);
	glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 3));
	glVertexAttribDivisor(11, 1);

	// Unbind the buffer and VAO to avoid accidental modification
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
/////////////////////////////////



/////////////////////////////////
// BuildGlyphInstances - Builds a vector of GPUTextGlyphInstance objects based on the provided CText and CTransform components, along with the specified FontAsset.
std::vector<GPUTextGlyphInstance> RenderSystemGL::BuildGlyphInstances(const CText& textComp, const CTransform& transformComp, const FontAsset& font) {
	std::vector<GPUTextGlyphInstance> glyphInstances;
	glyphInstances.reserve(textComp.text.size());

	if (!m_fontSystem) {
		std::cerr << "\x1b[91m[RenderSystemGL] Font system is not initialized\x1b[0m" << std::endl;
		return glyphInstances;
	}

	// ----------------------------------------------------
	// Split text into lines
	// ----------------------------------------------------
	std::vector<std::string> lines;
	{
		std::string current;
		for (char c : textComp.text) {
			if (c == '\n') {
				lines.push_back(current);
				current.clear();
			} else {
				current.push_back(c);
			}
		}
		lines.push_back(current);
	}

	// ----------------------------------------------------
	// Measure line width using xadvance (correct for alignment)
	// ----------------------------------------------------
	auto MeasureLine = [&](const std::string& line) {
		float width = 0.f;
		int prev = -1;
		bool first = true;

		for (char c : line) {
			const Glyph* g = m_fontSystem->GetGlyph(font, (int)c);
			if (!g)
				continue;

			if (prev >= 0)
				width += m_fontSystem->GetKerning(font, prev, (int)c);

			if (first) {
				width += g->xoffset; // IMPORTANT
				first = false;
			}

			width += g->xadvance;
			prev = (int)c;
		}

		return width;
	};

	// ----------------------------------------------------
	// Build glyphs line-by-line
	// ----------------------------------------------------
	float cursorY = transformComp.position.y;

	for (const std::string& line : lines) {
		int i = 0;

		float lineWidth = MeasureLine(line);

		// ----------------------------------------------------
		// Independent alignment per line
		// ----------------------------------------------------
		float cursorX = transformComp.position.x;

		switch (textComp.align) {
		case CText::Align::Center:
			cursorX -= lineWidth * 0.5f;
			break;

		case CText::Align::Right:
			cursorX -= lineWidth;
			break;

		default:
			break; // Left
		}

		int prevChar = -1;

		// ----------------------------------------------------
		// Glyph loop
		// ----------------------------------------------------
		for (char c : line) {

			const Glyph* glyph = m_fontSystem->GetGlyph(font, (int)c);
			if (!glyph)
				continue;

			if (prevChar >= 0)
				cursorX += m_fontSystem->GetKerning(font, prevChar, (int)c);

			GPUTextGlyphInstance gi{};

			gi.x = cursorX + glyph->xoffset;
			gi.y = cursorY + glyph->yoffset;

			gi.w = glyph->w;
			gi.h = glyph->h;

			float invW = 1.0f / font.scaleW;
			float invH = 1.0f / font.scaleH;

			gi.u0 = glyph->u0 * invW;
			gi.v0 = glyph->v0 * invH;
			gi.u1 = glyph->u1 * invW;
			gi.v1 = glyph->v1 * invH;

			gi.r = textComp.color.r / 255.f;
			gi.g = textComp.color.g / 255.f;
			gi.b = textComp.color.b / 255.f;
			gi.a = textComp.color.a / 255.f;

			glyphInstances.push_back(gi);

			cursorX += glyph->xadvance; // matches MeasureLine()
			prevChar = (int)c;
		}

		cursorY += font.lineHeight;
	}

	return glyphInstances;
}
/////////////////////////////////



/////////////////////////////////
// UploadGlyphInstances - Uploads the current glyph instances to the GPU for rendering. This method should be called after building the glyph instances and before rendering text.
void RenderSystemGL::UploadGlyphInstances() {
	if (m_textGlyphInstances.empty())
		return;

	// Bind instance buffer and upload all glyphs
	glBindBuffer(GL_ARRAY_BUFFER, m_textInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, m_textGlyphInstances.size() * sizeof(GPUTextGlyphInstance),
				 m_textGlyphInstances.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
/////////////////////////////////



/////////////////////////////////
// RenderTextGlyphs - Renders the uploaded glyph instances using instanced rendering. This method should be called after uploading the glyph instances to the GPU.
void RenderSystemGL::RenderTextGlyphs() {
	if (m_textGlyphInstances.empty())
		return;

	glUseProgram(m_textShaderProgram);

	if (m_textViewportUniformLocation >= 0) {
		glUniform2f(m_textViewportUniformLocation, static_cast<float>(m_viewportWidth),
					static_cast<float>(m_viewportHeight));
	}

	// Bind atlas from the same font used in BuildGlyphInstances
	const FontAsset* font = m_fontSystem->GetFont("default"); // or batch per fontKey
	if (font) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, font->textureID);
	} else {
		std::cerr << "\x1b[91m[RenderSystemGL]No font atlas bound for text rendering\x1b[0m\n";
	}

	glBindVertexArray(m_textVAO);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, m_textGlyphInstances.size());
	glBindVertexArray(0);

	glUseProgram(0);

	// Optional: clear after draw so next frame starts fresh
	m_textGlyphInstances.clear();
}
/////////////////////////////////



/////////////////////////////////
void RenderSystemGL::CreateTextGlyphResources() {
	glGenVertexArrays(1, &m_textGlyphVAO);
	glBindVertexArray(m_textGlyphVAO);

	glGenBuffers(1, &m_textGlyphVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_textGlyphVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GPUTextGlyphInstance) * 4096, nullptr, GL_DYNAMIC_DRAW);

	// pos + size (x,y,w,h)
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GPUTextGlyphInstance),
						  (void*)offsetof(GPUTextGlyphInstance, x));
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);

	// UVs (u0,v0,u1,v1)
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GPUTextGlyphInstance),
						  (void*)offsetof(GPUTextGlyphInstance, u0));
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 1);

	// color (r,g,b,a)
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(GPUTextGlyphInstance),
						  (void*)offsetof(GPUTextGlyphInstance, r));
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 1);

	glBindVertexArray(0);
}
/////////////////////////////////



/////////////////////////////////
void RenderSystemGL::LoadTextShader() {
	// We already created m_textShaderProgram in CreateTextResources()
	if (!m_textShaderProgram) {
		std::cerr << "\x1b[91m[RenderSystemGL]LoadTextShader called but m_textShaderProgram is null\x1b[0m\n";
		return;
	}

	glUseProgram(m_textShaderProgram);

	// Cache viewport uniform if not already
	m_textViewportUniformLocation = glGetUniformLocation(m_textShaderProgram, "uViewportSize");

	// Bind sampler to texture unit 0
	GLint atlasLoc = glGetUniformLocation(m_textShaderProgram, "uAtlas");
	if (atlasLoc >= 0) {
		glUniform1i(atlasLoc, 0); // uAtlas → GL_TEXTURE0
	} else {
		std::cerr << "\x1b[91m[RenderSystemGL] Failed to locate uAtlas uniform in text shader\x1b[0m\n";
	}

	glUseProgram(0);
}
/////////////////////////////////



/////////////////////////////////
// RenderSprites - Renders a batch of sprite instances using instanced rendering.
void RenderSystemGL::RenderSprites(const std::vector<GPUSpriteInstance>& instances) {
	// Ensure we have enough buffer capacity for the instances to be rendered
	if (instances.empty() || !m_spriteShaderProgram || !m_quadVAO) {
		return;
	}

	// Ensure the instance buffer has enough capacity for the number of instances to be rendered
	EnsureSpriteBufferCapacity(instances.size());

	// Upload instance data to the GPU
	glBindBuffer(GL_ARRAY_BUFFER, m_spriteInstanceVBO);

	glBufferSubData(GL_ARRAY_BUFFER, 0, instances.size() * sizeof(GPUSpriteInstance), instances.data());

	// Bind the shader program and set the viewport uniform
	glUseProgram(m_spriteShaderProgram);

	if (m_spriteViewportUniformLocation >= 0) {
		glUniform2f(m_spriteViewportUniformLocation, static_cast<float>(m_viewportWidth), static_cast<float>(m_viewportHeight));
	}

	// Ensure the white texture is bound
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_whiteTexture);

	// Bind the quad VAO for rendering
	glBindVertexArray(m_quadVAO);

	// Make sure the instance VBO is bound to the VAO (VAO remembers this from setup)
	glBindBuffer(GL_ARRAY_BUFFER, m_spriteInstanceVBO);

	// Draw the instances using instanced rendering
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, instances.size());
	glFlush();

	GLenum err = glGetError();
	(void)err;  // Suppress unused warning

	// Unbind the VAO to avoid accidental modification
	glBindVertexArray(0);

	// Unbind the shader program
	glUseProgram(0);

	// Unbind the buffer
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
/////////////////////////////////



/////////////////////////////////
// RenderCircles - Renders a batch of circle instances using instanced rendering.
void RenderSystemGL::RenderCircles(const std::vector<GPUCircleInstance>& instances) {
	// Ensure we have enough buffer capacity for the instances to be rendered
	if (instances.empty() || !m_circleShaderProgram || !m_circleVAO) return;

	// Ensure the instance buffer has enough capacity for the number of instances to be rendered
	EnsureCircleBufferCapacity(instances.size());

	// Upload instance data to the GPU
	glBindBuffer(GL_ARRAY_BUFFER, m_circleInstanceVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, instances.size() * sizeof(GPUCircleInstance), instances.data());

	// Bind the shader program and set the viewport uniform
	glUseProgram(m_circleShaderProgram);

	// Bind the circle VAO for rendering
	glBindVertexArray(m_circleVAO);

	// Draw the instances using instanced rendering
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, instances.size());

	// Unbind the VAO to avoid accidental modification
	glBindVertexArray(0);

	// Unbind the shader program
	glUseProgram(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind the buffer to avoid accidental modification
}
/////////////////////////////////



/////////////////////////////////
// RenderText - Renders a batch of text instances using instanced rendering. This method is a placeholder and does not perform any actions in this implementation.
void RenderSystemGL::RenderText(const std::vector<GPUTextInstance>& instances) {
	if (instances.empty() || !m_textShaderProgram || !m_textVAO)
		return;

	EnsureTextBufferCapacity(instances.size());

	glBindBuffer(GL_ARRAY_BUFFER, m_textInstanceVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, instances.size() * sizeof(GPUTextInstance), instances.data());

	glUseProgram(m_textShaderProgram);

	glBindVertexArray(m_textVAO);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, instances.size());
	glBindVertexArray(0);

	glUseProgram(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
/////////////////////////////////



/////////////////////////////////
// CreateShaderProgram - Compiles vertex and fragment shaders, links them into a shader program, and returns the program ID. If compilation or linking fails, it prints the error log and returns 0.
GLuint RenderSystemGL::CreateShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
	// ------------------------------------------------------------
	// Compile vertex shader
	// ------------------------------------------------------------
	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertShader, 1, &vertexSrc, nullptr);
	glCompileShader(vertShader);

	GLint vertStatus = 0;
	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &vertStatus);

	// Check for compilation errors
	if (vertStatus != GL_TRUE) {
		GLint logLength = 0;
		glGetShaderiv(vertShader, GL_INFO_LOG_LENGTH, &logLength);

		std::string log(logLength, '\0');
		glGetShaderInfoLog(vertShader, logLength, nullptr, log.data());

		printf("Vertex shader compilation failed:\n%s\n", log.c_str());
		glDeleteShader(vertShader);
		return 0;
	}

	// ------------------------------------------------------------
	// Compile fragment shader
	// ------------------------------------------------------------
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragShader, 1, &fragmentSrc, nullptr);
	glCompileShader(fragShader);

	// Check for compilation errors
	GLint fragStatus = 0;
	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &fragStatus);

	// Check for compilation errors
	if (fragStatus != GL_TRUE) {
		GLint logLength = 0;
		glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &logLength);

		std::string log(logLength, '\0');
		glGetShaderInfoLog(fragShader, logLength, nullptr, log.data());

		printf("Fragment shader compilation failed:\n%s\n", log.c_str());
		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
		return 0;
	}

	// ------------------------------------------------------------
	// Link program
	// ------------------------------------------------------------
	GLuint program = glCreateProgram();
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);
	glLinkProgram(program);

	GLint linkStatus = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

	if (linkStatus != GL_TRUE) {
		GLint logLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

		std::string log(logLength, '\0');
		glGetProgramInfoLog(program, logLength, nullptr, log.data());

		printf("Shader program linking failed:\n%s\n", log.c_str());

		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
		glDeleteProgram(program);
		return 0;
	}

	// ------------------------------------------------------------
	// Cleanup shader objects (they're now linked into the program)
	// ------------------------------------------------------------
	glDetachShader(program, vertShader);
	glDetachShader(program, fragShader);
	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	return program;
}
/////////////////////////////////