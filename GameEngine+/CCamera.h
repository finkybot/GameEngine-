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
// CCamera component -	Represents a camera in the game world, with properties for position, zoom, rotation, viewport size, and camera shake effects.
//						|
//						|___________________________________________________________________________________
class CCamera : public Component {
	/////////////////////////////////
	// Public member variables for the CCamera component
public:
	/////////////////////////////////
	// Constructors for the CCamera component, default and parameterized.
	CCamera() = default;
	CCamera(const Vec2& position, float zoom) : position(position), zoom(zoom) {}
	/////////////////////////////////



	/////////////////////////////////
	// Public camera properties.
	Vec2 position = {0.0f, 0.0f};
	float zoom = 1.0f;				// Default zoom level (1.0 = no zoom)
	float rotation = 0.0f;			// Default rotation angle in degrees (0.0 = no rotation)
	float viewportWidth = 800.0f;		// Default viewport width in world units
	float viewportHeight = 600.0f;	// Default viewport height in world units
	
	bool isMainCamera = false;		// Flag to indicate if this camera is the main camera for rendering
	bool isActive = false;			// Flag to indicate if this camera is active and should be used for rendering
	
	float shakeMagnitude = 0.0f;		// Magnitude of camera shake effect (0.0 = no shake)
	float shakeDuration = 0.0f;		// Duration of camera shake effect in seconds

	float smoothness = 0.1f;			// Smoothness factor for camera movement (0.0 = instant, higher values = smoother)
};
/////////////////////////////////