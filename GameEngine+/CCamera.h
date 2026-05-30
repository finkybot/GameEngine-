/////////////////////////////////
// CCamera.h
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations for the CCamera component. We include necessary headers for component handling and 2D vector math.
#pragma once
#include "Component.h"
#include "Vec2.h"
/////////////////////////////////



/////////////////////////////////
// CCamera component - represents a camera in the game world, with properties for position, zoom, rotation, viewport size, and camera shake effects.
class CCamera : public Component {
public:
	CCamera() = default;
	CCamera(const Vec2& position, float zoom) : m_position(position), m_zoom(zoom) {}

	Vec2 m_position = {0.0f, 0.0f};
	float m_zoom = 1.0f; // Default zoom level (1.0 = no zoom)
	float m_rotation = 0.0f; // Default rotation angle in degrees (0.0 = no rotation)
	float m_viewportWidth = 800.0f; // Default viewport width in world units
	float m_viewportHeight = 600.0f; // Default viewport height in world units
	
	bool m_isMainCamera = false;	 // Flag to indicate if this camera is the main camera for rendering
	bool m_isActive = false;		 // Flag to indicate if this camera is active and should be used for rendering
	
	float m_shakeMagnitude = 0.0f;	 // Magnitude of camera shake effect (0.0 = no shake)
	float m_shakeDuration = 0.0f;	 // Duration of camera shake effect in seconds

	float m_smoothness = 0.1f; // Smoothness factor for camera movement (0.0 = instant, higher values = smoother)
};
/////////////////////////////////