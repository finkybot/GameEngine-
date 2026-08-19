/////////////////////////////////
// CameraSystem.h - Header file for the CameraSystem class, which manages camera components in the game engine. This system is responsible for updating camera positions, applying camera shake effects, and ensuring that the main camera is properly set up for rendering. 
// It interacts with the EntityManager to access entities with CCamera components and updates their states based on game logic and player input.
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations for the CameraSystem class. We include necessary headers for SFML graphics, the CCamera component, the EntityManager, and the CTransform component. We also include the optional header for returning optional values from methods.
#pragma once
#include <SFML/Graphics.hpp>
#include "CCamera.h"
#include "EntityManager.h"
#include "CTransform.h"
#include <optional>
/////////////////////////////////



/////////////////////////////////
// CameraSystem - A system responsible for managing camera components in the game engine. This system handles updating camera positions, applying camera shake effects, and ensuring that the main camera is properly set up for rendering. 
// It interacts with the EntityManager to access entities with CCamera components and updates their states based on game logic and player input.
//								|
//								|_______________________________________________________________________
class CameraSystem {
	/////////////////////////////////
	// Public interface for the CameraSystem class, including methods for updating camera states, retrieving the main camera, applying camera shake effects, setting the main camera, clearing the main camera, and configuring camera properties such as smoothness, viewport size, and target following.
public:
	void Update(float deltaTime, EntityManager& entityManager);
	std::optional<CCamera*> GetMainCamera(EntityManager& entityManager) const;
	void ApplyCameraShake(CCamera& camera, float magnitude, float duration);
	void SetMainCamera(EntityManager& entityManager, Entity* cameraEntity);
	void ClearMainCamera(EntityManager& entityManager);
	void SetCameraSmoothness(EntityManager& entityManager, float smoothness);
	void SetCameraViewportSize(EntityManager& entityManager, float width, float height);
	void SetCameraOnEntity(EntityManager& entityManager, Entity* cameraEntity, const Vec2& position, float zoom, float rotation);
	void UpdateCameraPosition(EntityManager& entityManager, Entity* cameraEntity, const Vec2& targetPosition, float deltaTime);
	void UpdateCameraRotation(EntityManager& entityManager, Entity* cameraEntity, float targetRotation, float deltaTime);
	void UpdateCameraZoom(EntityManager& entityManager, Entity* cameraEntity, float targetZoom, float deltaTime);
	void DeactivateCamera(EntityManager& entityManager, Entity* cameraEntity);
	void ActivateCamera(EntityManager& entityManager, Entity* cameraEntity);

	// Camera panning helpers (screen-space drag -> world-space camera movement)
	void BeginPan(CCamera& camera, const sf::Vector2i& mousePos);
	void UpdatePan(CCamera& camera, const sf::Vector2i& mousePos);
	void EndPan(CCamera& camera);
	bool IsPanning(const CCamera& camera) const;

	// Utility clamp for scene-driven cameras/views: clamps camera center to map bounds using viewport and zoom.
	static Vec2 ClampPositionToBounds(const Vec2& position, float viewportWidth, float viewportHeight, float zoom,
									 const Vec2& mapMin, const Vec2& mapMax, bool hasBounds);

	// Apply camera state to an SFML view and optional bounds clamp in one place.
	static sf::View BuildViewFromCamera(const CCamera& camera, bool clampToBounds, const Vec2& mapMin, const Vec2& mapMax,
									 bool hasBounds);
	/////////////////////////////////
};
/////////////////////////////////