/////////////////////////////////
// Implementation of GPURenderSystem
/////////////////////////////////



/////////////////////////////////
// Includes
#include "GPURenderSystem.h"
#include "ShutdownGuard.h"
#include <iostream>
/////////////////////////////////



/////////////////////////////////
GPURenderSystem::GPURenderSystem() {}
/////////////////////////////////



/////////////////////////////////
GPURenderSystem::~GPURenderSystem() {
	if (!ShutdownGuard::IsShuttingDown()) Shutdown();
}
/////////////////////////////////



/////////////////////////////////
// Shutdown releases all OpenGL resources. It should be called while a valid OpenGL context is active.
void GPURenderSystem::Shutdown() {
	// Release OpenGL resources VAOs, VBOs, and shader programs
	if (m_quadVBO) {
		glDeleteBuffers(1, &m_quadVBO);
		m_quadVBO = 0;
	}

	if (m_explosionInstanceVBO) {
		glDeleteBuffers(1, &m_explosionInstanceVBO);
		m_explosionInstanceVBO = 0;
	}

	if (m_explosionVAO) {
		glDeleteVertexArrays(1, &m_explosionVAO);
		m_explosionVAO = 0;
	}

	if (m_explosionShaderProgram) {
		glDeleteProgram(m_explosionShaderProgram);
		m_explosionShaderProgram = 0;
	}

	if (m_equalizerInstanceVBO) {
		glDeleteBuffers(1, &m_equalizerInstanceVBO);
		m_equalizerInstanceVBO = 0;
	}

	if (m_equalizerVAO) {
		glDeleteVertexArrays(1, &m_equalizerVAO);
		m_equalizerVAO = 0;
	}

	if (m_equalizerShaderProgram) {
		glDeleteProgram(m_equalizerShaderProgram);
		m_equalizerShaderProgram = 0;
	}

	m_explosionViewportUniformLocation = -1;
	m_equalizerViewportUniformLocation = -1;
	m_explosionBufferCapacity = 0;
	m_equalizerBufferCapacity = 0;
	m_initialized = false;
}
/////////////////////////////////



/////////////////////////////////
// Initialize sets up OpenGL resources. It should be called once after GLAD is initialized and a valid OpenGL context is active.
void GPURenderSystem::Initialize() {
	// Check if already initialized, get out if so
	if (m_initialized) return;

	// Check for OpenGL context and required functions
	const GLubyte* version = glGetString(GL_VERSION);
	if (!version) {
		std::cerr << "[GPURenderSystem] No active OpenGL context during Initialize()" << std::endl;
		return;
	}

	// Check for required OpenGL functions for instancing
	if (!glGenVertexArrays || !glBindVertexArray || !glVertexAttribDivisor || !glDrawArraysInstanced) {
		std::cerr << "[GPURenderSystem] Required OpenGL instancing functions are unavailable on this context" << std::endl;
		return;
	}

	// Everything looks good, lets create resources
	CreateBuffers();
	CreateQuadGeometry();
	CreateExplosionResources();
	CreateEqualizerResources();
	CreateCircleResources();

	if (!m_quadVBO || !m_explosionVAO || !m_explosionInstanceVBO || !m_explosionShaderProgram || m_explosionViewportUniformLocation < 0 ||
		!m_equalizerVAO || !m_equalizerInstanceVBO || !m_equalizerShaderProgram || m_equalizerViewportUniformLocation < 0) {
		Shutdown();
		return;
	}

	m_initialized = true;
}
/////////////////////////////////



/////////////////////////////////
void GPURenderSystem::CreateBuffers() {
	// Create shared quad buffer
	glGenBuffers(1, &m_quadVBO);
	if (!m_quadVBO) {
		std::cerr << "[GPURenderSystem] Failed to create shared quad buffer" << std::endl;
	}
}
/////////////////////////////////



/////////////////////////////////
// CreateQuadGeometry creates a simple quad geometry in normalized device coordinates (NDC) for instanced rendering.
void GPURenderSystem::CreateQuadGeometry() {
	// Quad vertices in Normalised Device Coordinates (NDC) space (x, y), we cover the full range (screen) of [-1, 1] in both axes. 
	// The vertex shader will scale and translate these based on instance data.
	const float quadVerts[] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f};
	//const float quadVerts[] = {-0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};

	// Upload quad vertices to the shared quad VBO
	glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
/////////////////////////////////



/////////////////////////////////
// CreateExplosionResources creates the OpenGL resources needed for rendering explosion effects, including shaders, VAO, and instance VBO.
void GPURenderSystem::CreateExplosionResources() {
	// Vertex shader vs
	const char* vs = R"(
		#version 330 core

		layout(location = 0) in vec2 quadPos;
		layout(location = 1) in vec4 posRad;
		layout(location = 2) in vec4 lifeCol;
		layout(location = 3) in float alpha;

		uniform vec2 uViewportSize;

		out vec2 fragPos;
		out float vRadius;
		out float vAge;
		out float vLifetime;
		out vec4 vColor;

		void main()
		{
			fragPos = quadPos * posRad.z;
			vRadius = posRad.z;
			vAge = posRad.w;
			vLifetime = lifeCol.x;
			vColor = vec4(lifeCol.yzw, alpha);

			vec2 centerNdc = vec2(
				(posRad.x / uViewportSize.x) * 2.0 - 1.0,
				1.0 - (posRad.y / uViewportSize.y) * 2.0
			);

			vec2 localNdc = vec2(
				(fragPos.x / uViewportSize.x) * 2.0,
				-(fragPos.y / uViewportSize.y) * 2.0
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
	m_explosionShaderProgram = CreateShaderProgram(vs, fs);
	if (!m_explosionShaderProgram) return;

	// Get uniform location for viewport size and check for errors
	m_explosionViewportUniformLocation = glGetUniformLocation(m_explosionShaderProgram, "uViewportSize");
	if (m_explosionViewportUniformLocation < 0) {
		std::cerr << "[GPURenderSystem] Failed to locate explosion viewport uniform" << std::endl;
		return;
	}

	// Create VAO and instance VBO for explosions and check for errors
	glGenVertexArrays(1, &m_explosionVAO);
	glGenBuffers(1, &m_explosionInstanceVBO);
	if (!m_explosionVAO || !m_explosionInstanceVBO) {
		std::cerr << "[GPURenderSystem] Failed to create explosion GL resources" << std::endl;
		return;
	}

	// Setup VAO for instanced rendering
	glBindVertexArray(m_explosionVAO);

	// Bind the shared quad VBO and set up vertex attribute for quad positions
	glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
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
void GPURenderSystem::CreateEqualizerResources() {
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
	m_equalizerShaderProgram = CreateShaderProgram(vs, fs);
	if (!m_equalizerShaderProgram) return;

	// Get uniform location for viewport size and check for errors
	m_equalizerViewportUniformLocation = glGetUniformLocation(m_equalizerShaderProgram, "uViewportSize");
	if (m_equalizerViewportUniformLocation < 0) {
		std::cerr << "[GPURenderSystem] Failed to locate equalizer viewport uniform" << std::endl;
		return;
	}

	// Create VAO and instance VBO for equalizer bars and check for errors
	glGenVertexArrays(1, &m_equalizerVAO);
	glGenBuffers(1, &m_equalizerInstanceVBO);
	if (!m_equalizerVAO || !m_equalizerInstanceVBO) {
		std::cerr << "[GPURenderSystem] Failed to create equalizer GL resources" << std::endl;
		return;
	}

	// Setup VAO for instanced rendering of equalizer bars
	glBindVertexArray(m_equalizerVAO);

	// Bind the shared quad VBO and set up vertex attribute for quad positions
	glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
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
void GPURenderSystem::CreateCircleResources() {
	// --- Circle Vertex Shader ---
	const char* vs = R"(
        #version 330 core

        layout(location = 0) in vec2 quadPos;      // shared quad [-1,1]
        layout(location = 1) in vec4 posRad;       // x, y, radius, unused
        layout(location = 2) in vec4 color;        // r, g, b, a

        uniform vec2 uViewportSize;

        out vec2 fragPos;
        out float vRadius;
        out vec4 vColor;

        void main()
        {
            // quadPos scaled by radius → local pixel-space circle coords
            fragPos = quadPos * posRad.z;
            vRadius = posRad.z;
            vColor = color;

            // Convert center from pixel → NDC
            vec2 centerNdc = vec2(
                (posRad.x / uViewportSize.x) * 2.0 - 1.0,
                1.0 - (posRad.y / uViewportSize.y) * 2.0
            );

            // Convert local offset from pixel → NDC
            vec2 localNdc = vec2(
                (fragPos.x / uViewportSize.x) * 2.0,
                -(fragPos.y / uViewportSize.y) * 2.0
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

	m_circleShaderProgram = CreateShaderProgram(vs, fs);
	if (!m_circleShaderProgram)
		return;

	m_circleViewportUniformLocation = glGetUniformLocation(m_circleShaderProgram, "uViewportSize");
	if (m_circleViewportUniformLocation < 0) {
		std::cerr << "[GPURenderSystem] Failed to locate circle viewport uniform\n";
		return;
	}

	glGenVertexArrays(1, &m_circleVAO);
	glGenBuffers(1, &m_circleInstanceVBO);
	if (!m_circleVAO || !m_circleInstanceVBO) {
		std::cerr << "[GPURenderSystem] Failed to create circle GL resources\n";
		return;
	}

	glBindVertexArray(m_circleVAO);

	// shared quad
	glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);

	// instance buffer uses GPUShapeInstance
	glBindBuffer(GL_ARRAY_BUFFER, m_circleInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GPUShapeInstance), nullptr, GL_DYNAMIC_DRAW);
	m_circleBufferCapacity = 1;

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
// PrepareViewport retrieves the current OpenGL viewport dimensions and ensures they are valid. If the viewport dimensions are invalid, it falls back to cached values.
bool GPURenderSystem::PrepareViewport(int viewport[4]) const {
	// Check for active OpenGL context
	if (!glGetString(GL_VERSION)) {
		std::cerr << "[GPURenderSystem] No active OpenGL context during Render()" << std::endl;
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
		std::cerr << "[GPURenderSystem] Invalid viewport dimensions during Render()" << std::endl;
		return false;
	}

	// Cache the viewport dimensions for future use
	return true;
}
/////////////////////////////////



/////////////////////////////////
// EnsureExplosionBufferCapacity checks if the current explosion instance buffer has enough capacity for the required number of instances. If not, it reallocates the buffer with the new capacity.
void GPURenderSystem::EnsureExplosionBufferCapacity(std::size_t requiredInstances) {
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
void GPURenderSystem::EnsureEqualizerBufferCapacity(std::size_t requiredInstances) {
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
// EnsureCircleBufferCapacity checks if the current circle instance buffer has enough capacity for the required number of instances. If not, it reallocates the buffer with the new capacity.
void GPURenderSystem::EnsureCircleBufferCapacity(std::size_t requiredInstances) {
	// If the required number of instances is less than or equal to the current buffer capacity, no action is needed.
	if (requiredInstances <= m_circleBufferCapacity) return;

	// Update the buffer capacity to the required number of instances
	m_circleBufferCapacity = requiredInstances;
	glBindBuffer(GL_ARRAY_BUFFER, m_circleInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_circleBufferCapacity * sizeof(GPUShapeInstance)), nullptr, GL_DYNAMIC_DRAW);
}
/////////////////////////////////



/////////////////////////////////
// RenderShapes renders the shape instances using instanced rendering. It takes a vector of GPUShapeInstance representing the shape instances to render.
void GPURenderSystem::RenderShapes(const std::vector<GPUShapeInstance>& instances) {
	if (!m_initialized || instances.empty())
		return;

	//std::cout << "[GPURenderSystem] Rendering " << instances.size() << " GPU shape instances" << std::endl;

	// Convert GPUShapeInstance → GPUInstanceData
	std::vector<GPUInstanceData> converted;
	converted.reserve(instances.size());

	for (const auto& s : instances) {
		converted.push_back(GPUInstanceData{s.x, s.y, s.radius, s.r, s.g, s.b, s.a});
	}

	// Ensure buffer capacity for GPUInstanceData
	EnsureExplosionBufferCapacity(converted.size());

	glBindVertexArray(m_explosionVAO);
	glUseProgram(m_explosionShaderProgram);

	// Set viewport uniform so the shader can convert pixel coords → NDC
	GLint viewport[4] = {0, 0, 0, 0};
	glGetIntegerv(GL_VIEWPORT, viewport);
	glUniform2f(m_explosionViewportUniformLocation, static_cast<float>(viewport[2]), static_cast<float>(viewport[3]));

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	glBindBuffer(GL_ARRAY_BUFFER, m_explosionInstanceVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(converted.size() * sizeof(GPUInstanceData)), converted.data());

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(converted.size()));
}
/////////////////////////////////



/////////////////////////////////
void GPURenderSystem::RenderCircles(const std::vector<GPUShapeInstance>& instances) {
	if (!m_initialized || instances.empty())
		return;

	EnsureCircleBufferCapacity(instances.size());

	glBindVertexArray(m_circleVAO);
	glUseProgram(m_circleShaderProgram);

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	glUniform2f(m_circleViewportUniformLocation, static_cast<float>(viewport[2]), static_cast<float>(viewport[3]));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindBuffer(GL_ARRAY_BUFFER, m_circleInstanceVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(instances.size() * sizeof(GPUShapeInstance)),
					instances.data());

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(instances.size()));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}
/////////////////////////////////



/////////////////////////////////
// RenderExplosions renders the explosion effects using instanced rendering. It takes a vector of GPUInstanceData representing the explosion instances to render.
void GPURenderSystem::RenderExplosions(const std::vector<GPUInstanceData>& instances) {
	// Check if the renderer is initialized and if there are valid resources and instances to render
	if (!m_initialized || !m_explosionVAO || !m_explosionInstanceVBO || !m_explosionShaderProgram || m_explosionViewportUniformLocation < 0 || instances.empty())
		return;


	// Prepare the viewport dimensions for rendering
	GLint viewport[4] = {0, 0, 0, 0};

	// If the viewport preparation fails, exit early
	if (!PrepareViewport(viewport))	return;

	// Enable blending for transparency and set the blend function
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Use the explosion shader program and set the viewport uniform
	glUseProgram(m_explosionShaderProgram);
	glUniform2f(m_explosionViewportUniformLocation, static_cast<float>(viewport[2]), static_cast<float>(viewport[3]));
	glBindVertexArray(m_explosionVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_explosionInstanceVBO);
	EnsureExplosionBufferCapacity(instances.size());
	glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(instances.size() * sizeof(GPUInstanceData)), instances.data());
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(instances.size()));

	// Unbind buffers and VAO to avoid accidental modification
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}
/////////////////////////////////



/////////////////////////////////
// RenderEqualizerBars renders the equalizer bars using instanced rendering. It takes a vector of GPUBarInstanceData representing the equalizer bar instances to render.
void GPURenderSystem::RenderEqualizerBars(const std::vector<GPUBarInstanceData>& instances) {
	// Check if the renderer is initialized and if there are valid resources and instances to render
	if (!m_initialized || !m_equalizerVAO || !m_equalizerInstanceVBO || !m_equalizerShaderProgram || m_equalizerViewportUniformLocation < 0 || instances.empty())
		return;

	// Prepare the viewport dimensions for rendering
	GLint viewport[4] = {0, 0, 0, 0};

	// If the viewport preparation fails, exit early
	if (!PrepareViewport(viewport))	return;

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
void GPURenderSystem::OnResize(int width, int height) {
	// Update cached viewport dimensions
	m_viewportWidth = width;
	m_viewportHeight = height;
}
/////////////////////////////////



/////////////////////////////////
// CreateShaderProgram compiles and links a vertex and fragment shader into a shader program. It returns the program ID on success or 0 on failure.
GLuint GPURenderSystem::CreateShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
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
		std::cerr << "[GPURenderSystem] Vertex shader compilation failed: " << infoLog << std::endl;
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
		std::cerr << "[GPURenderSystem] Fragment shader compilation failed: " << infoLog << std::endl;
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
		std::cerr << "[GPURenderSystem] Shader program link failed: " << infoLog << std::endl;
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