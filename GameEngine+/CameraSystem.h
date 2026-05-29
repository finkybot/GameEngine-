// ***** CameraSystem.h *****
#pragma once
#include <SFML/Graphics.hpp>

#include "CCamera.h"
#include "EntityManager.h"
#include "CTransform.h"

#include <optional>

// CameraSystem - A system responsible for managing camera components in the game engine. This system handles updating camera positions, applying camera shake effects, and ensuring that the main camera is properly set up for rendering. 
// It interacts with the EntityManager to access entities with CCamera components and updates their states based on game logic and player input.
class CameraSystem {
public:
	void Update(float deltaTime, EntityManager& entityManager);
	std::optional<CCamera*> GetMainCamera(EntityManager& entityManager) const;
	void ApplyCameraShake(CCamera& camera, float magnitude, float duration);
	void SetMainCamera(EntityManager& entityManager, Entity* cameraEntity);
	void ClearMainCamera(EntityManager& entityManager);
	void SetCameraSmoothness(EntityManager& entityManager, float smoothness);
	void SetCameraViewportSize(EntityManager& entityManager, float width, float height);
	void SetCameraOnEntity(EntityManager& entityManager, Entity* cameraEntity, const Vec2& position, float zoom,
						   float rotation);
	void UpdateCameraPosition(EntityManager& entityManager, Entity* cameraEntity, const Vec2& targetPosition,
							  float deltaTime);
	void UpdateCameraRotation(EntityManager& entityManager, Entity* cameraEntity, float targetRotation,
							  float deltaTime);
	void UpdateCameraZoom(EntityManager& entityManager, Entity* cameraEntity, float targetZoom, float deltaTime);
	void DeactivateCamera(EntityManager& entityManager, Entity* cameraEntity);
	void ActivateCamera(EntityManager& entityManager, Entity* cameraEntity);
};