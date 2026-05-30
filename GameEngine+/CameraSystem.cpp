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
/////////////////////////////////




/////////////////////////////////
// Static map to track shake data for each camera. This allows us to store the original position, magnitude, and duration of the shake effect for each camera that is currently shaking.
static std::unordered_map<CCamera*, ShakeData> s_shakeDataMap;
/////////////////////////////////
 
 

/////////////////////////////////
// Random number generator for shake offsets, why mt19937? (I don't fucking know but AI recommended it, saying it's a good general-purpose RNG with better randomness quality than rand()).
static std::mt19937 s_rng(std::random_device{}());
/////////////////////////////////



/////////////////////////////////
// Update method for the CameraSystem. This method is called every frame and is responsible for updating the position of active cameras based on their shake effects and smooth following behavior.
void CameraSystem::Update(float deltaTime, EntityManager& entityManager) {
	// Update camera shake timers and smoothing for active cameras
	for (auto& uniquePtr : entityManager.getEntities()) {
		Entity* entity = uniquePtr.get(); // Get raw pointer from unique_ptr for easier access
		
		// Check if the entity has a CCamera component and if it's active (We only want to apply shake to active cameras)
		if (auto camera = entity->GetComponent<CCamera>()) {
			if (camera->m_isActive) {
				auto it = s_shakeDataMap.find(camera); // Look up shake data for this camera	
				if (it != s_shakeDataMap.end()) {	   // If shake data exists for this camera, update the shake effect
					ShakeData& shakeData = it->second; // Reference to the shake data for easier access
					
					// Update shake timer and calculate new camera position with shake offset
					if (shakeData.duration > 0.0f) { 
						shakeData.duration -= deltaTime;
						float currentMagnitude = shakeData.magnitude * (shakeData.duration / shakeData.magnitude); // Linear falloff
						std::uniform_real_distribution<float> dist(-currentMagnitude, currentMagnitude);
						camera->m_position = shakeData.basePosition + Vec2(dist(s_rng), dist(s_rng));
					} else {
						camera->m_position = shakeData.basePosition; // Reset to original position after shake ends
						s_shakeDataMap.erase(it);
					}
				}
			}
		}

		// If the camera is following an entity, update its position based on the target's position and the camera's smoothness factor
		if (auto tform = entity->GetComponent<CTransform>()) {
			if (auto camera = entity->GetComponent<CCamera>()) {
				if (camera->m_isActive) {
					Vec2 targetPosition = tform->m_position; // Get the target position from the transform component
					camera->m_position += (targetPosition - camera->m_position) * camera->m_smoothness; // Smoothly move towards the target position
				}
			}
		}
	}
}


// Method to get the main camera from the EntityManager. This iterates through all entities and checks for a CCamera component with the m_isMainCamera flag set to true. 
// Returns an optional pointer to the main camera, or std::nullopt if no main camera is found.
std::optional<CCamera*> CameraSystem::GetMainCamera(EntityManager& entityManager) const {
	for (auto& uniquePtr : entityManager.getEntities()) {
		Entity* entity = uniquePtr.get(); // Get raw pointer from unique_ptr for easier access
		if (auto camera = entity->GetComponent<CCamera>()) {
			if (camera->m_isMainCamera) {
				return camera; // Return the main camera if found
			}
		}
	}
	return std::nullopt; // Return nullopt if no main camera is found
}


// Method to apply a camera shake effect to a specific camera. This creates a ShakeData struct with the camera's current position, 
// the specified magnitude and duration, and stores it in the s_shakeDataMap for processing in the Update method.
void CameraSystem::ApplyCameraShake(CCamera& camera, float magnitude, float duration) {
	ShakeData shakeData;
	shakeData.basePosition = camera.m_position;
	shakeData.magnitude = magnitude;
	shakeData.duration = duration;
	s_shakeDataMap[&camera] = shakeData;
}


// Method to set a specific camera entity as the main camera. This iterates through all entities and sets the m_isMainCamera flag on the specified camera entity, while clearing it on all other cameras.	
void CameraSystem::SetMainCamera(EntityManager& entityManager, Entity* cameraEntity) {
	if (!cameraEntity) return; // Guard: If the provided camera entity is null, do nothing

	for (auto& uniquePtr : entityManager.getEntities()) {
		Entity* entity = uniquePtr.get(); // Get raw pointer from unique_ptr for easier access
		if (auto camera = entity->GetComponent<CCamera>()) {
			camera->m_isMainCamera = (entity == cameraEntity); // Set m_isMainCamera to true for the specified camera entity, false for all others
		}
	}
}


// Method to clear the main camera designation from all cameras. This iterates through all entities and sets the m_isMainCamera flag to false on any entity with a CCamera component.
void CameraSystem::ClearMainCamera(EntityManager& entityManager) {
	for (auto& uniquePtr : entityManager.getEntities()) {
		Entity* entity = uniquePtr.get(); // Get raw pointer from unique_ptr for easier access
		if (auto camera = entity->GetComponent<CCamera>()) {
			camera->m_isMainCamera = false; // Clear the main camera designation
		}
	}
}


// Method to set the smoothness factor for a specific camera. This allows for adjusting how smoothly the camera follows its target position. 
// Higher values result in smoother movement, while a value of 0 results in instant movement.
void CameraSystem::SetCameraSmoothness(EntityManager& entityManager, float smoothness) {
	for (auto& uniquePtr : entityManager.getEntities()) {
		Entity* entity = uniquePtr.get(); // Get raw pointer from unique_ptr for easier access
		if (auto camera = entity->GetComponent<CCamera>()) {
			camera->m_smoothness = smoothness; // Set the smoothness factor for the camera
		}
	}
}


// Method to set the viewport size for a specific camera. This allows for adjusting the area of the game world that the camera can see.
void CameraSystem::SetCameraViewportSize(EntityManager& entityManager, float width, float height) {
	for (auto& uniquePtr : entityManager.getEntities()) {
		Entity* entity = uniquePtr.get(); // Get raw pointer from unique_ptr for easier access
		if (auto camera = entity->GetComponent<CCamera>()) {
			camera->m_viewportWidth = width; // Set the viewport width for the camera
			camera->m_viewportHeight = height; // Set the viewport height for the camera
		}
	}
}


// Method to set the camera's position, zoom, and rotation based on a target entity. This allows for quickly configuring a camera to follow a specific entity with the desired settings.
void CameraSystem::SetCameraOnEntity(EntityManager& entityManager, Entity* cameraEntity, const Vec2& position, float zoom, float rotation) {
	if (!cameraEntity)	return; // Guard: If the provided camera entity is null, do nothing
	if (auto camera = cameraEntity->GetComponent<CCamera>()) {
		camera->m_position = position; // Set the camera's position to the specified position
		camera->m_zoom = zoom;		   // Set the camera's zoom level to the specified zoom
		camera->m_rotation = rotation; // Set the camera's rotation angle to the specified rotation
	}
}


// Method to update the camera's position smoothly towards a target position. This is typically called in the Update method to move the camera towards 
// its target entity's position over time, using the camera's smoothness factor for interpolation.
void CameraSystem::UpdateCameraPosition(EntityManager& entityManager, Entity* cameraEntity, const Vec2& targetPosition,	float deltaTime) {
	if (!cameraEntity) return; // Guard: If the provided camera entity is null, do nothing
	if (auto camera = cameraEntity->GetComponent<CCamera>()) {
		// Smoothly interpolate the camera's position towards the target position using the smoothness factor
		camera->m_position = camera->m_position + (targetPosition - camera->m_position) * camera->m_smoothness * deltaTime;
	}
}


// Method to update the camera's rotation smoothly towards a target rotation. This is typically called in the Update method to rotate the camera towards
void CameraSystem::UpdateCameraRotation(EntityManager& entityManager, Entity* cameraEntity, float targetRotation, float deltaTime) {
	if (!cameraEntity)	return; // Guard: If the provided camera entity is null, do nothing
	if (auto camera = cameraEntity->GetComponent<CCamera>()) {
		// Smoothly interpolate the camera's rotation towards the target rotation using the smoothness factor
		camera->m_rotation = camera->m_rotation + (targetRotation - camera->m_rotation) * camera->m_smoothness * deltaTime;
	}
}


// Method to update the camera's zoom level smoothly towards a target zoom. This is typically called in the Update method 
// to zoom the camera in or out towards a desired zoom level over time, using the camera's smoothness factor for interpolation.
void CameraSystem::UpdateCameraZoom(EntityManager& entityManager, Entity* cameraEntity, float targetZoom, float deltaTime) {
	if (!cameraEntity)	return; // Guard: If the provided camera entity is null, do nothing
	if (auto camera = cameraEntity->GetComponent<CCamera>()) {
		// Smoothly interpolate the camera's zoom towards the target zoom using the smoothness factor
		camera->m_zoom = camera->m_zoom + (targetZoom - camera->m_zoom) * camera->m_smoothness * deltaTime;
	}
}


// Method to deactivate a camera, which sets its active flag to false. This can be used to temporarily disable a camera without destroying it, allowing for easy reactivation later.
void CameraSystem::DeactivateCamera(EntityManager& entityManager, Entity* cameraEntity) {
	if (!cameraEntity)	return; // Guard: If the provided camera entity is null, do nothing
	auto camera = cameraEntity->GetComponent<CCamera>();
	if (camera) {
		camera->m_isActive = false; // Set the camera's active flag to false to deactivate it
	}
}


// Method to activate a camera, which sets its active flag to true. This can be used to enable a camera that was previously deactivated, allowing it to be used for rendering again.
void CameraSystem::ActivateCamera(EntityManager& entityManager, Entity* cameraEntity) {
	if (!cameraEntity)	return; // Guard: If the provided camera entity is null, do nothing
	auto camera = cameraEntity->GetComponent<CCamera>();
	if (camera) {
		camera->m_isActive = true; // Set the camera's active flag to true to activate it
	}
}
