#include "RenderSystemGL.h"
#include "ShutdownGuard.h"
#include <iostream>

void RenderSystemGL::CreateGpuEffectBuffers() {
	// Create shared quad buffer
	glGenBuffers(1, &m_gpuQuadVBO);
	if (!m_gpuQuadVBO) {
		std::cerr << "[RenderSystemGL] Failed to create shared quad buffer" << std::endl;
	}
}
/////////////////////////////////



/////////////////////////////////
// CreateGpuEffectQuadGeometry creates a simple quad geometry in normalized device coordinates (NDC) for instanced rendering.
void RenderSystemGL::CreateGpuEffectQuadGeometry() {
	// Quad vertices in Normalised Device Coordinates (NDC) space (x, y), we cover the full range (screen) of [-1, 1] in both axes. 
	// The vertex shader will scale and translate these based on instance data.
	const float quadVerts[] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f};
	//const float quadVerts[] = {-0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};

	// Upload quad vertices to the shared quad VBO
	glBindBuffer(GL_ARRAY_BUFFER, m_gpuQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
/////////////////////////////////



/////////////////////////////////
// CreateExplosionResources creates the OpenGL resources needed for rendering explosion effects, including shaders, VAO, and instance VBO.
void RenderSystemGL::CreateExplosionResources() {
	// Vertex shader vs
	const char* vs = R"(

		#version 330 core

		layout(location = 0) in vec2 quadPos;
		layout(location = 1) in vec4 posRad;   // x, y, radius, age
		layout(location = 2) in vec4 lifeCol;  // lifetime, r, g, b
		layout(location = 3) in float alpha;   // alpha

		uniform vec2  uViewportSize;
		uniform vec2  uCameraPos;
		uniform float uCameraZoom;
		//uniform float uWorldWidth;
		//uniform float uWorldHeight;

		out vec2 fragPos;
		out float vRadius;
		out float vAge;
		out float vLifetime;
		out vec4 vColor;

		void main()
		{
			fragPos   = quadPos * posRad.z;
			vRadius   = posRad.z;
			vAge      = posRad.w;
			vLifetime = lifeCol.x;
			vColor    = vec4(lifeCol.yzw, alpha);

			// world-space position relative to camera
			vec2 worldPos = posRad.xy - uCameraPos;

			// world scaled by zoom
			float scaledW = uViewportSize.x / uCameraZoom;
			float scaledH = uViewportSize.y / uCameraZoom;


			vec2 centerNdc = vec2(
				(worldPos.x / scaledW) * 2.0,
				-(worldPos.y / scaledH) * 2.0
			);

			vec2 localNdc = vec2(
				(fragPos.x / scaledW) * 2.0,
				-(fragPos.y / scaledH) * 2.0
			);

			gl_Position = vec4(centerNdc + localNdc, 0.0, 1.0);
		}
	)";


	// Fragment shader fs
	const char* fs = R"(
		#version 330 core

		in vec2 fragPos;
		in float vRadius;
		in float vAge;
		in float vLifetime;
		in vec4 vColor;

		out vec4 fragColor;

		void main()
		{
			float dist = length(fragPos);
			if (dist > vRadius)
				discard;

			float alpha = 1.0 - (dist / vRadius);
			float fade = 1.0 - (vAge / vLifetime);
			fragColor = vec4(vColor.rgb, vColor.a * alpha * fade);
		}
	)";

	// Create shader program and check for errors
	m_explosionShaderProgram = CreateGpuShaderProgram(vs, fs);
	if (!m_explosionShaderProgram) return;

	// Get uniform location for viewport size and check for errors
	m_explosionViewportUniformLocation = glGetUniformLocation(m_explosionShaderProgram, "uViewportSize");
	//if (m_explosionViewportUniformLocation < 0) {
	//	std::cerr << "[RenderSystemGL] Failed to locate explosion viewport uniform" << std::endl;
	//	return;
	//}

	// Create VAO and instance VBO for explosions and check for errors
	glGenVertexArrays(1, &m_explosionVAO);
	glGenBuffers(1, &m_explosionInstanceVBO);
	if (!m_explosionVAO || !m_explosionInstanceVBO) {
		std::cerr << "[RenderSystemGL] Failed to create explosion GL resources" << std::endl;
		return;
	}

	// Setup VAO for instanced rendering
	glBindVertexArray(m_explosionVAO);

	// Bind the shared quad VBO and set up vertex attribute for quad positions
	glBindBuffer(GL_ARRAY_BUFFER, m_gpuQuadVBO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);

	// Bind the instance VBO and allocate initial buffer data for explosion instances
	glBindBuffer(GL_ARRAY_BUFFER, m_explosionInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GPUInstanceData), nullptr, GL_DYNAMIC_DRAW);
	m_explosionBufferCapacity = 1;

	// Set up vertex attributes for instance data: position/radius, life/color, and alpha
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GPUInstanceData), (void*)0);
	glVertexAttribDivisor(1, 1);
	glEnableVertexAttribArray(1);

	// Set up vertex attributes for instance data: color and alpha
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GPUInstanceData), (void*)(sizeof(float) * 4));
	glVertexAttribDivisor(2, 1);
	glEnableVertexAttribArray(2);

	// Set up vertex attribute for alpha
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(GPUInstanceData), (void*)(sizeof(float) * 8));
	glVertexAttribDivisor(3, 1);
	glEnableVertexAttribArray(3);

	// Unbind buffers and VAO to avoid accidental modification
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
/////////////////////////////////



/////////////////////////////////
// CreateEqualizerResources creates the OpenGL resources needed for rendering equalizer bars, including shaders, VAO, and instance VBO.
void RenderSystemGL::CreateEqualizerResources() {
	const char* vs = R"(
		#version 330 core

		layout(location = 0) in vec2 quadPos;
		layout(location = 1) in vec4 posSize;
		layout(location = 2) in vec4 color;

		uniform vec2 uViewportSize;

		out vec4 vColor;

		void main()
		{
			vec2 localPos = vec2(quadPos.x * posSize.z, quadPos.y * posSize.w);
			vec2 screenPos = posSize.xy + localPos;
			vec2 ndc = vec2(
				(screenPos.x / uViewportSize.x) * 2.0 - 1.0,
				1.0 - (screenPos.y / uViewportSize.y) * 2.0
			);

			vColor = color;
			gl_Position = vec4(ndc, 0.0, 1.0);
		}
	)";

	const char* fs = R"(
		#version 330 core

		in vec4 vColor;
		out vec4 fragColor;

		void main()
		{
			fragColor = vColor;
		}
	)";

	// Create shader program for equalizer bars and check for errors
	m_equalizerShaderProgram = CreateGpuShaderProgram(vs, fs);
	if (!m_equalizerShaderProgram) return;

	// Get uniform location for viewport size and check for errors
	m_equalizerViewportUniformLocation = glGetUniformLocation(m_equalizerShaderProgram, "uViewportSize");
	if (m_equalizerViewportUniformLocation < 0) {
		std::cerr << "[RenderSystemGL] Failed to locate equalizer viewport uniform" << std::endl;
		return;
	}

	// Create VAO and instance VBO for equalizer bars and check for errors
	glGenVertexArrays(1, &m_equalizerVAO);
	glGenBuffers(1, &m_equalizerInstanceVBO);
	if (!m_equalizerVAO || !m_equalizerInstanceVBO) {
		std::cerr << "[RenderSystemGL] Failed to create equalizer GL resources" << std::endl;
		return;
	}

	// Setup VAO for instanced rendering of equalizer bars
	glBindVertexArray(m_equalizerVAO);

	// Bind the shared quad VBO and set up vertex attribute for quad positions
	glBindBuffer(GL_ARRAY_BUFFER, m_gpuQuadVBO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);

	// Bind the instance VBO and allocate initial buffer data for equalizer bar instances
	glBindBuffer(GL_ARRAY_BUFFER, m_equalizerInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GPUBarInstanceData), nullptr, GL_DYNAMIC_DRAW);
	m_equalizerBufferCapacity = 1;

	// Set up vertex attributes for instance data: position/size and color
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GPUBarInstanceData), (void*)0);
	glVertexAttribDivisor(1, 1);
	glEnableVertexAttribArray(1);

	// Set up vertex attributes for instance data: color
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GPUBarInstanceData), (void*)(sizeof(float) * 4));
	glVertexAttribDivisor(2, 1);
	glEnableVertexAttribArray(2);

	// Unbind buffers and VAO to avoid accidental modification
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
//////////////////////////////////


//////////////////////////////////
void RenderSystemGL::CreateGpuCircleResources() {
	// --- Circle Vertex Shader ---
	const char* vs = R"(
		#version 330 core

		layout(location = 0) in vec2 quadPos;      // shared quad [-1,1]
		layout(location = 1) in vec4 posRad;       // x, y, radius, unused
		layout(location = 2) in vec4 color;        // r, g, b, a

		uniform vec2  uViewportSize;
		uniform vec2  uCameraPos;
		uniform float uCameraZoom;
		//uniform float uWorldWidth;
		//uniform float uWorldHeight;

		out vec2 fragPos;
		out float vRadius;
		out vec4 vColor;

		void main()
		{
			fragPos = quadPos * posRad.z;
			vRadius = posRad.z;
			vColor  = color;

			// world-space position relative to camera
			vec2 worldPos = posRad.xy - uCameraPos;

			// scale world by zoom
			float scaledW = uViewportSize.x / uCameraZoom;
			float scaledH = uViewportSize.y / uCameraZoom;


			vec2 centerNdc = vec2(
				(worldPos.x / scaledW) * 2.0,
				-(worldPos.y / scaledH) * 2.0
			);

			vec2 localNdc = vec2(
				(fragPos.x / scaledW) * 2.0,
				-(fragPos.y / scaledH) * 2.0
			);

			gl_Position = vec4(centerNdc + localNdc, 0.0, 1.0);
		}
    )";

	// --- Circle Fragment Shader ---
	const char* fs = R"(
        #version 330 core

        in vec2 fragPos;
        in float vRadius;
        in vec4 vColor;

        out vec4 fragColor;

        void main()
        {
            float dist = length(fragPos);
            if (dist > vRadius)
                discard;

            float alpha = 1.0 - (dist / vRadius);
            fragColor = vec4(vColor.rgb, vColor.a * alpha);
        }
    )";
	// ... same shaders ...

	m_gpuCircleShaderProgram = CreateGpuShaderProgram(vs, fs);
	if (!m_gpuCircleShaderProgram)
		return;

	m_gpuCircleViewportUniformLocation = glGetUniformLocation(m_gpuCircleShaderProgram, "uViewportSize");
	//if (m_gpuCircleViewportUniformLocation < 0) {
	//	std::cerr << "[RenderSystemGL] Failed to locate circle viewport uniform\n";
	//	return;
	//}

	glGenVertexArrays(1, &m_gpuCircleVAO);
	glGenBuffers(1, &m_gpuCircleInstanceVBO);
	if (!m_gpuCircleVAO || !m_gpuCircleInstanceVBO) {
		std::cerr << "[RenderSystemGL] Failed to create circle GL resources\n";
		return;
	}

	glBindVertexArray(m_gpuCircleVAO);

	// shared quad
	glBindBuffer(GL_ARRAY_BUFFER, m_gpuQuadVBO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);

	// instance buffer uses GPUShapeInstance
	glBindBuffer(GL_ARRAY_BUFFER, m_gpuCircleInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GPUShapeInstance), nullptr, GL_DYNAMIC_DRAW);
	m_gpuCircleBufferCapacity = 1;

	// posRad: x, y, radius, unused
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GPUShapeInstance), (void*)0);
	glVertexAttribDivisor(1, 1);
	glEnableVertexAttribArray(1);

	// color: r, g, b, a (offset 3 floats)
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GPUShapeInstance), (void*)(sizeof(float) * 3));
	glVertexAttribDivisor(2, 1);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
//////////////////////////////////


/////////////////////////////////
// PrepareGpuViewport retrieves the current OpenGL viewport dimensions and ensures they are valid. If the viewport dimensions are invalid, it falls back to cached values.
bool RenderSystemGL::PrepareGpuViewport(int viewport[4]) const {
	// Check for active OpenGL context
	if (!glGetString(GL_VERSION)) {
		std::cerr << "[RenderSystemGL] No active OpenGL context during Render()" << std::endl;
		return false;
	}

	// Retrieve the current viewport dimensions
	glGetIntegerv(GL_VIEWPORT, viewport);
	if (viewport[2] <= 0 || viewport[3] <= 0) {
		viewport[2] = m_viewportWidth;
		viewport[3] = m_viewportHeight;
	}

	// Validate the viewport dimensions
	if (viewport[2] <= 0 || viewport[3] <= 0) {
		std::cerr << "[RenderSystemGL] Invalid viewport dimensions during Render()" << std::endl;
		return false;
	}

	// Cache the viewport dimensions for future use
	return true;
}
/////////////////////////////////



/////////////////////////////////
// EnsureExplosionBufferCapacity checks if the current explosion instance buffer has enough capacity for the required number of instances. If not, it reallocates the buffer with the new capacity.
void RenderSystemGL::EnsureExplosionBufferCapacity(std::size_t requiredInstances) {
	// If the required number of instances is less than or equal to the current buffer capacity, no action is needed.
	if (requiredInstances <= m_explosionBufferCapacity)	return;

	// Update the buffer capacity to the required number of instances
	m_explosionBufferCapacity = requiredInstances;
	glBindBuffer(GL_ARRAY_BUFFER, m_explosionInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER,
		static_cast<GLsizeiptr>(m_explosionBufferCapacity * sizeof(GPUInstanceData)),
		nullptr,
		GL_DYNAMIC_DRAW);
}
/////////////////////////////////



/////////////////////////////////
// EnsureEqualizerBufferCapacity checks if the current equalizer instance buffer has enough capacity for the required number of instances. If not, it reallocates the buffer with the new capacity.
void RenderSystemGL::EnsureEqualizerBufferCapacity(std::size_t requiredInstances) {
	// If the required number of instances is less than or equal to the current buffer capacity, no action is needed.
	if (requiredInstances <= m_equalizerBufferCapacity)	return;

	// Update the buffer capacity to the required number of instances
	m_equalizerBufferCapacity = requiredInstances;
	glBindBuffer(GL_ARRAY_BUFFER, m_equalizerInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER,
		static_cast<GLsizeiptr>(m_equalizerBufferCapacity * sizeof(GPUBarInstanceData)),
		nullptr,
		GL_DYNAMIC_DRAW);
}
/////////////////////////////////



/////////////////////////////////
// EnsureGpuCircleBufferCapacity checks if the current circle instance buffer has enough capacity for the required number of instances. If not, it reallocates the buffer with the new capacity.
void RenderSystemGL::EnsureGpuCircleBufferCapacity(std::size_t requiredInstances) {
	// If the required number of instances is less than or equal to the current buffer capacity, no action is needed.
	if (requiredInstances <= m_gpuCircleBufferCapacity) return;

	// Update the buffer capacity to the required number of instances
	m_gpuCircleBufferCapacity = requiredInstances;
	glBindBuffer(GL_ARRAY_BUFFER, m_gpuCircleInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_gpuCircleBufferCapacity * sizeof(GPUShapeInstance)), nullptr, GL_DYNAMIC_DRAW);
}
/////////////////////////////////



/////////////////////////////////
// RenderShapes renders the shape instances using instanced rendering. It takes a vector of GPUShapeInstance representing the shape instances to render.
void RenderSystemGL::RenderShapes(const std::vector<GPUShapeInstance>& instances, const CCamera& cam) {
	// Check if the renderer is initialized and if there are valid instances to render
	if (!m_initialised || instances.empty())
		return;

	//std::cout << "[RenderSystemGL] Rendering " << instances.size() << " GPU shape instances" << std::endl;

	std::vector<GPUShapeInstance> visibleInstances;
	visibleInstances.reserve(instances.size());
	for (const auto& instance : instances) {
		if (IsVisible(cam, Vec2(instance.x, instance.y), instance.radius, instance.radius))
			visibleInstances.push_back(instance);
	}
	if (visibleInstances.empty())
		return;

	// Convert GPUShapeInstance → GPUInstanceData
	std::vector<GPUInstanceData> converted;
	converted.reserve(visibleInstances.size());

	for (const auto& s : visibleInstances) {
		converted.push_back(GPUInstanceData{s.x, s.y, s.radius, s.r, s.g, s.b, s.a});
	}

	// Ensure buffer capacity for GPUInstanceData
	EnsureExplosionBufferCapacity(converted.size());

	glBindVertexArray(m_explosionVAO);
	glUseProgram(m_explosionShaderProgram);

	// Set viewport uniform so the shader can convert pixel coords → NDC
	GLint viewport[4] = {0, 0, 0, 0};
	glGetIntegerv(GL_VIEWPORT, viewport);
	//glUniform2f(m_explosionViewportUniformLocation, static_cast<float>(viewport[2]), static_cast<float>(viewport[3]));

	
    // NEW: camera uniforms
	GLint camPosLoc = glGetUniformLocation(m_explosionShaderProgram, "uCameraPos");
	GLint camZoomLoc = glGetUniformLocation(m_explosionShaderProgram, "uCameraZoom");

	glUniform2f(camPosLoc, cam.position.x, cam.position.y);
	glUniform1f(camZoomLoc, cam.zoom);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	glBindBuffer(GL_ARRAY_BUFFER, m_explosionInstanceVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(converted.size() * sizeof(GPUInstanceData)), converted.data());

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(converted.size()));
}
/////////////////////////////////



/////////////////////////////////
void RenderSystemGL::RenderGpuCircles(const std::vector<GPUShapeInstance>& instances, const CCamera& cam) {
	if (!m_initialised || instances.empty())
		return;

	std::vector<GPUShapeInstance> visibleInstances;
	visibleInstances.reserve(instances.size());
	for (const auto& instance : instances) {
		if (IsVisible(cam, Vec2(instance.x, instance.y), instance.radius, instance.radius))
			visibleInstances.push_back(instance);
	}
	if (visibleInstances.empty())
		return;

	EnsureGpuCircleBufferCapacity(visibleInstances.size());

	glBindVertexArray(m_gpuCircleVAO);
	glUseProgram(m_gpuCircleShaderProgram);

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	//glUniform2f(m_gpuCircleViewportUniformLocation, static_cast<float>(viewport[2]), static_cast<float>(viewport[3]));

	// NEW: camera uniforms
	// NEW: camera uniforms
	GLint camPosLoc = glGetUniformLocation(m_gpuCircleShaderProgram, "uCameraPos");
	GLint camZoomLoc = glGetUniformLocation(m_gpuCircleShaderProgram, "uCameraZoom");
	GLint viewWLoc = glGetUniformLocation(m_gpuCircleShaderProgram, "uViewportSize");



	glUniform2f(camPosLoc, cam.position.x, cam.position.y);
	glUniform1f(camZoomLoc, cam.zoom);
	glUniform2f(viewWLoc, cam.viewportWidth, cam.viewportHeight);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindBuffer(GL_ARRAY_BUFFER, m_gpuCircleInstanceVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(visibleInstances.size() * sizeof(GPUShapeInstance)),
					visibleInstances.data());

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(visibleInstances.size()));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}
/////////////////////////////////



/////////////////////////////////
// RenderExplosions renders the explosion effects using instanced rendering. It takes a vector of GPUInstanceData representing the explosion instances to render.
void RenderSystemGL::RenderExplosions(const std::vector<GPUInstanceData>& instances, const CCamera& cam) {
	// Check if the renderer is initialized and if there are valid resources and instances to render
	if (!m_initialised || !m_explosionVAO || !m_explosionInstanceVBO || !m_explosionShaderProgram || instances.empty())
		return;


	std::vector<GPUInstanceData> visibleInstances;
	visibleInstances.reserve(instances.size());
	for (const auto& instance : instances) {
		if (IsVisible(cam, Vec2(instance.x, instance.y), instance.radius, instance.radius))
			visibleInstances.push_back(instance);
	}
	if (visibleInstances.empty())
		return;

	// Prepare the viewport dimensions for rendering
	GLint viewport[4] = {0, 0, 0, 0};
	if (!PrepareGpuViewport(viewport))
		return;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(m_explosionShaderProgram);
	glBindVertexArray(m_explosionVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_explosionInstanceVBO);

	// viewport uniform
	//glUniform2f(m_explosionViewportUniformLocation, static_cast<float>(viewport[2]), static_cast<float>(viewport[3]));

	// camera uniforms (must match shader names)
	GLint camPosLoc = glGetUniformLocation(m_explosionShaderProgram, "uCameraPos");
	GLint camZoomLoc = glGetUniformLocation(m_explosionShaderProgram, "uCameraZoom");
	GLint viewWLoc = glGetUniformLocation(m_explosionShaderProgram, "uViewportSize");


	glUniform2f(camPosLoc, cam.position.x, cam.position.y);
	glUniform1f(camZoomLoc, cam.zoom);
	glUniform2f(viewWLoc, cam.viewportWidth, cam.viewportHeight);
	//glUniform1f(worldWLoc, cam.worldWidth);
	//glUniform1f(worldHLoc, cam.worldHeight);

	EnsureExplosionBufferCapacity(visibleInstances.size());
	glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(visibleInstances.size() * sizeof(GPUInstanceData)),
					visibleInstances.data());

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(visibleInstances.size()));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}
/////////////////////////////////



/////////////////////////////////
// RenderEqualizerBars renders the equalizer bars using instanced rendering. It takes a vector of GPUBarInstanceData representing the equalizer bar instances to render.
void RenderSystemGL::RenderEqualizerBars(const std::vector<GPUBarInstanceData>& instances) {
	// Check if the renderer is initialized and if there are valid resources and instances to render
	if (!m_initialised || !m_equalizerVAO || !m_equalizerInstanceVBO || !m_equalizerShaderProgram || m_equalizerViewportUniformLocation < 0 || instances.empty())
		return;

	// Prepare the viewport dimensions for rendering
	GLint viewport[4] = {0, 0, 0, 0};

	// If the viewport preparation fails, exit early
	if (!PrepareGpuViewport(viewport))	return;

	// Enable blending for transparency and set the blend function
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Use the equalizer shader program and set the viewport uniform
	glUseProgram(m_equalizerShaderProgram);
	glUniform2f(m_equalizerViewportUniformLocation, static_cast<float>(viewport[2]), static_cast<float>(viewport[3]));
	glBindVertexArray(m_equalizerVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_equalizerInstanceVBO);
	EnsureEqualizerBufferCapacity(instances.size());
	glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(instances.size() * sizeof(GPUBarInstanceData)), instances.data());
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(instances.size()));

	// Unbind buffers and VAO to avoid accidental modification
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}
/////////////////////////////////



/////////////////////////////////
// OnResize updates the cached viewport dimensions when the window is resized. This ensures that the renderer uses the correct dimensions for rendering.
GLuint RenderSystemGL::CreateGpuShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
	// Create vertex shader vs
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vertexSrc, nullptr);
	glCompileShader(vs);

	// Check for compilation errors in the vertex shader
	GLint compileStatus = GL_FALSE;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &compileStatus);

	// If compilation failed, retrieve and log the error message, then clean up and return 0
	if (compileStatus != GL_TRUE) {
		GLchar infoLog[1024] = {};
		glGetShaderInfoLog(vs, sizeof(infoLog), nullptr, infoLog);
		std::cerr << "[RenderSystemGL] Vertex shader compilation failed: " << infoLog << std::endl;
		glDeleteShader(vs);
		return 0;
	}

	// Create fragment shader fs
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fragmentSrc, nullptr);
	glCompileShader(fs);\

	// Check for compilation errors in the fragment shader
	glGetShaderiv(fs, GL_COMPILE_STATUS, &compileStatus);

	// If compilation failed, retrieve and log the error message, then clean up and return 0
	if (compileStatus != GL_TRUE) {
		GLchar infoLog[1024] = {};
		glGetShaderInfoLog(fs, sizeof(infoLog), nullptr, infoLog);
		std::cerr << "[RenderSystemGL] Fragment shader compilation failed: " << infoLog << std::endl;
		glDeleteShader(vs);
		glDeleteShader(fs);
		return 0;
	}

	// Create shader program and link shaders
	GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);

	//	
	GLint linkStatus = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

	// If linking failed, retrieve and log the error message, then clean up and return 0
	if (linkStatus != GL_TRUE) {
		GLchar infoLog[1024] = {};
		glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
		std::cerr << "[RenderSystemGL] Shader program link failed: " << infoLog << std::endl;
		glDeleteProgram(program);
		glDeleteShader(vs);
		glDeleteShader(fs);
		return 0;
	}

	// Clean up shaders after linking, as they are no longer needed once linked into the program
	glDeleteShader(vs);
	glDeleteShader(fs);
	return program;
}
/////////////////////////////////
