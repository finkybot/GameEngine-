/////////////////////////////////
// CameraSystem.cpp
/////////////////////////////////



/////////////////////////////////
// Includes and namespace aliases for the CameraSystem implementation. We include necessary headers for the camera system, entity management, and random number generation for camera shake effects.
#include "CameraSystem.h"
#include "Entity.h"
#include <unordered_map>
#include <random>
/////////////////////////////////




/////////////////////////////////
// Struct to hold shake data for each camera. This includes the base position of the camera before shaking, the magnitude of the shake effect, and the remaining duration of the shake effect.
struct ShakeData {
	Vec2 basePosition;
	float magnitude = 0.0f;
	float duration = 0.0f;
};

struct PanData {
	bool active = false;
	sf::Vector2i panStart = sf::Vector2i(0, 0);
	Vec2 camStart = Vec2::Zero;
};
/////////////////////////////////




/////////////////////////////////
// Static map to track shake data for each camera. This allows us to store the original position, magnitude, and duration of the shake effect for each camera that is currently shaking.
static std::unordered_map<CCamera*, ShakeData> s_shakeDataMap;
static std::unordered_map<CCamera*, PanData> s_panDataMap;
/////////////////////////////////



/////////////////////////////////
// Random number generator for shake offsets, why mt19937? (I don't fucking know but AI recommended it, saying it's a good general-purpose RNG with better randomness quality than rand()).
static std::mt19937 s_rng(std::random_device{}());
/////////////////////////////////



/////////////////////////////////
// Update - Called every frame and is responsible for updating the position of active cameras based on their shake effects and smooth following behavior.
Vec2 CameraSystem::ClampPositionToBounds(const Vec2& position, float viewportWidth, float viewportHeight, float zoom,
										 const Vec2& mapMin, const Vec2& mapMax, bool hasBounds) {
	if (!hasBounds)
		return position;

	Vec2 out = position;
	const float safeZoom = (zoom > 0.0001f) ? zoom : 1.0f;
	const float halfW = (viewportWidth / safeZoom) * 0.5f;
	const float halfH = (viewportHeight / safeZoom) * 0.5f;
	const float minCx = mapMin.x + halfW;
	const float maxCx = mapMax.x - halfW;
	const float minCy = mapMin.y + halfH;
	const float maxCy = mapMax.y - halfH;

	if (minCx <= maxCx)
		out.x = std::clamp(out.x, minCx, maxCx);
	if (minCy <= maxCy)
		out.y = std::clamp(out.y, minCy, maxCy);
	return out;
}
/////////////////////////////////



/////////////////////////////////
sf::View CameraSystem::BuildViewFromCamera(const CCamera& camera, bool clampToBounds, const Vec2& mapMin, const Vec2& mapMax,
										   bool hasBounds) {
	const float safeZoom = (camera.zoom > 0.0001f) ? camera.zoom : 1.0f;
	Vec2 center = camera.position;
	if (clampToBounds)
		center = ClampPositionToBounds(center, camera.viewportWidth, camera.viewportHeight, safeZoom, mapMin, mapMax, hasBounds);

	sf::View view;
	view.setSize(sf::Vector2f(camera.viewportWidth / safeZoom, camera.viewportHeight / safeZoom));
	view.setCenter(sf::Vector2f(center.x, center.y));
	return view;
}
/////////////////////////////////



///////////////////////////////
void CameraSystem::Update(float deltaTime, EntityManager& entityManager) {
	for (auto& uniquePtr : entityManager.GetEntities()) {
		Entity* entity = uniquePtr.get();
		if (!entity)
			continue;

		auto camera = entity->GetComponent<CCamera>();
		if (!camera || !camera->isActive)
			continue;

		// ---------------------------------------------------------
		// 1. FOLLOW TARGET (smooth)
		// ---------------------------------------------------------
		if (auto tform = entity->GetComponent<CTransform>()) {
			Vec2 target = tform->position;

			float t = 1.0f - std::exp(-camera->smoothness * deltaTime);
			camera->position += (target - camera->position) * t;
		}

		// ---------------------------------------------------------
		// 2. CLAMP (world bounds, zoom‑correct)
		// ---------------------------------------------------------
		float halfW_screen = camera->viewportWidth * 0.5f;
		float halfH_screen = camera->viewportHeight * 0.5f;

		float minX = halfW_screen / camera->zoom;
		float minY = halfH_screen / camera->zoom;

		camera->position.x = std::max(camera->position.x, minX);
		camera->position.y = std::max(camera->position.y, minY);

		// ---------------------------------------------------------
		// 3. SHAKE (apply AFTER clamping)
		// ---------------------------------------------------------
		auto it = s_shakeDataMap.find(camera);
		if (it != s_shakeDataMap.end()) {
			ShakeData& shakeData = it->second;

			if (shakeData.duration > 0.0f) {
				shakeData.duration -= deltaTime;

				float currentMagnitude = shakeData.magnitude * (shakeData.duration / shakeData.magnitude);

				std::uniform_real_distribution<float> dist(-currentMagnitude, currentMagnitude);

				// Apply shake as an offset AFTER clamping
				camera->position += Vec2(dist(s_rng), dist(s_rng));
			} else {
				s_shakeDataMap.erase(it);
			}
		}
		// ---------------------------------------------------------
		// 4. PIXEL SNAP (fix shimmering)
		// ---------------------------------------------------------
		float snap = 1.0f / camera->zoom;
		camera->position.x = std::round(camera->position.x / snap) * snap;
		camera->position.y = std::round(camera->position.y / snap) * snap;

	}
}
/////////////////////////////////



/////////////////////////////////
// GetMainCamera - Handles retrieving the main camera from the EntityManager. This iterates through all entities and checks for a CCamera component with the m_isMainCamera flag set to true. 
// Returns an optional pointer to the main camera, or std::nullopt if no main camera is found.
std::optional<CCamera*> CameraSystem::GetMainCamera(EntityManager& entityManager) const {
	for (auto& uniquePtr : entityManager.GetEntities()) {
		Entity* entity = uniquePtr.get(); // Get raw pointer from unique_ptr for easier access
		if (auto camera = entity->GetComponent<CCamera>()) {
			if (camera->isMainCamera) {
				return camera; // Return the main camera if found
			}
		}
	}
	return std::nullopt; // Return nullopt if no main camera is found
}
/////////////////////////////////



/////////////////////////////////
// ApplyCameraShake - Applies a camera shake effect to a specific camera. This creates a ShakeData struct with the camera's current position, 
// the specified magnitude and duration, and stores it in the s_shakeDataMap for processing in the Update method.
void CameraSystem::ApplyCameraShake(CCamera& camera, float magnitude, float duration) {
	ShakeData shakeData;
	shakeData.basePosition = camera.position;
	shakeData.magnitude = magnitude;
	shakeData.duration = duration;
	s_shakeDataMap[&camera] = shakeData;
}
/////////////////////////////////



/////////////////////////////////
void CameraSystem::BeginPan(CCamera& camera, const sf::Vector2i& mousePos) {
	PanData& pan = s_panDataMap[&camera];
	pan.active = true;
	pan.panStart = mousePos;
	pan.camStart = camera.position;
}
/////////////////////////////////



/////////////////////////////////
void CameraSystem::UpdatePan(CCamera& camera, const sf::Vector2i& mousePos) {
	auto it = s_panDataMap.find(&camera);
	if (it == s_panDataMap.end() || !it->second.active)
		return;

	const sf::Vector2i delta = mousePos - it->second.panStart;
	camera.position = it->second.camStart - Vec2(static_cast<float>(delta.x), static_cast<float>(delta.y));
}
/////////////////////////////////



/////////////////////////////////
void CameraSystem::EndPan(CCamera& camera) {
	auto it = s_panDataMap.find(&camera);
	if (it == s_panDataMap.end())
		return;
	it->second.active = false;
}
/////////////////////////////////



/////////////////////////////////
bool CameraSystem::IsPanning(const CCamera& camera) const {
	auto it = s_panDataMap.find(const_cast<CCamera*>(&camera));
	return (it != s_panDataMap.end()) && it->second.active;
}
/////////////////////////////////



/////////////////////////////////
// SetMainCamera - Sets a specific camera entity as the main camera. This iterates through all entities and sets the m_isMainCamera flag on the specified camera entity, while clearing it on all other cameras.	
void CameraSystem::SetMainCamera(EntityManager& entityManager, Entity* cameraEntity) {
	if (!cameraEntity) return; // Guard: If the provided camera entity is null, do nothing

	for (auto& uniquePtr : entityManager.GetEntities()) {
		Entity* entity = uniquePtr.get(); // Get raw pointer from unique_ptr for easier access
		if (auto camera = entity->GetComponent<CCamera>()) {
			camera->isMainCamera = (entity == cameraEntity); // Set isMainCamera to true for the specified camera entity, false for all others
		}
	}
}
/////////////////////////////////
 


/////////////////////////////////
// ClearMainCamera - Clears the main camera designation from all cameras. This iterates through all entities and sets the isMainCamera flag to false on any entity with a CCamera component.
void CameraSystem::ClearMainCamera(EntityManager& entityManager) {
	// Clear shake and pan data
	s_shakeDataMap.clear();
	s_panDataMap.clear();

	// Clear main camera flags on all camera components
	for (auto& uniquePtr : entityManager.GetEntities()) {
		if (auto camera = uniquePtr->GetComponent<CCamera>()) {
			camera->isMainCamera = false;
			camera->isActive = false;
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// SetCameraSmoothness - Sets the smoothness factor for a specific camera. This allows for adjusting how smoothly the camera follows its target position. 
// Higher values result in smoother movement, while a value of 0 results in instant movement.
void CameraSystem::SetCameraSmoothness(EntityManager& entityManager, float smoothness) {
	for (auto& uniquePtr : entityManager.GetEntities()) {
		Entity* entity = uniquePtr.get(); // Get raw pointer from unique_ptr for easier access
		if (auto camera = entity->GetComponent<CCamera>()) {
			camera->smoothness = smoothness; // Set the smoothness factor for the camera
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// SetCameraViewportSize - Sets the viewport size for a specific camera. This allows for adjusting the area of the game world that the camera can see.
void CameraSystem::SetCameraViewportSize(EntityManager& entityManager, float width, float height) {
	for (auto& uniquePtr : entityManager.GetEntities()) {
		Entity* entity = uniquePtr.get(); // Get raw pointer from unique_ptr for easier access
		if (auto camera = entity->GetComponent<CCamera>()) {
			camera->viewportWidth = width; // Set the viewport width for the camera
			camera->viewportHeight = height; // Set the viewport height for the camera
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// SetCameraOnEntity - Sets the camera's position, zoom, and rotation based on a target entity. This allows for quickly configuring a camera to follow a specific entity with the desired settings.
void CameraSystem::SetCameraOnEntity(EntityManager& entityManager, Entity* cameraEntity, const Vec2& position, float zoom, float rotation) {
	if (!cameraEntity)	return; // Guard: If the provided camera entity is null, do nothing
	if (auto camera = cameraEntity->GetComponent<CCamera>()) {
		camera->position = position; // Set the camera's position to the specified position
		camera->zoom = zoom;		   // Set the camera's zoom level to the specified zoom
		camera->rotation = rotation; // Set the camera's rotation angle to the specified rotation
	}
}
/////////////////////////////////



/////////////////////////////////
// UpdateCameraPosition - Updates the camera's position smoothly towards a target position. This is typically called in the Update method to move the camera towards 
// its target entity's position over time, using the camera's smoothness factor for interpolation.
void CameraSystem::UpdateCameraPosition(EntityManager& entityManager, Entity* cameraEntity, const Vec2& targetPosition,	float deltaTime) {
	if (!cameraEntity) return; // Guard: If the provided camera entity is null, do nothing
	if (auto camera = cameraEntity->GetComponent<CCamera>()) {
		// Smoothly interpolate the camera's position towards the target position using the smoothness factor
		camera->position = camera->position + (targetPosition - camera->position) * camera->smoothness * deltaTime;
	}
}
/////////////////////////////////



/////////////////////////////////
// UpdateCameraRotation - Updates the camera's rotation smoothly towards a target rotation. This is typically called in the Update method to rotate the camera towards
void CameraSystem::UpdateCameraRotation(EntityManager& entityManager, Entity* cameraEntity, float targetRotation, float deltaTime) {
	if (!cameraEntity)	return; // Guard: If the provided camera entity is null, do nothing
	if (auto camera = cameraEntity->GetComponent<CCamera>()) {
		// Smoothly interpolate the camera's rotation towards the target rotation using the smoothness factor
		camera->rotation = camera->rotation + (targetRotation - camera->rotation) * camera->smoothness * deltaTime;
	}
}
/////////////////////////////////



/////////////////////////////////
// UpdateCameraZoom - Updates the camera's zoom level smoothly towards a target zoom. This is typically called in the Update method 
// to zoom the camera in or out towards a desired zoom level over time, using the camera's smoothness factor for interpolation.
void CameraSystem::UpdateCameraZoom(EntityManager& entityManager, Entity* cameraEntity, float targetZoom, float deltaTime) {
	if (!cameraEntity)	return; // Guard: If the provided camera entity is null, do nothing
	if (auto camera = cameraEntity->GetComponent<CCamera>()) {
		// Smoothly interpolate the camera's zoom towards the target zoom using the smoothness factor
		camera->zoom = camera->zoom + (targetZoom - camera->zoom) * camera->smoothness * deltaTime;
	}
}
/////////////////////////////////



/////////////////////////////////
// DeactivateCamera - Deactivates a camera by setting its active flag to false. This can be used to temporarily disable a camera without destroying it, allowing for easy reactivation later.
void CameraSystem::DeactivateCamera(EntityManager& entityManager, Entity* cameraEntity) {
	if (!cameraEntity)	return; // Guard: If the provided camera entity is null, do nothing
	auto camera = cameraEntity->GetComponent<CCamera>();
	if (camera) {
		camera->isActive = false; // Set the camera's active flag to false to deactivate it
	}
}
/////////////////////////////////



/////////////////////////////////
// ActivateCamera - Activates a camera by setting its active flag to true. This can be used to enable a camera that was previously deactivated, allowing it to be used for rendering again.
void CameraSystem::ActivateCamera(EntityManager& entityManager, Entity* cameraEntity) {
	if (!cameraEntity)	return; // Guard: If the provided camera entity is null, do nothing
	auto camera = cameraEntity->GetComponent<CCamera>();
	if (camera) {
		camera->isActive = true; // Set the camera's active flag to true to activate it
	}
}
/////////////////////////////////