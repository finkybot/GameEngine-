/////////////////////////////////
// CColliderRect.h - Defines a simple structure for a rectangular collider with width and height properties, thats it nothing more
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
/////////////////////////////////



/////////////////////////////////
// CColliderRect - A simple structure representing a rectangular collider with width and height properties.
//								|
//								|_______________________________________________________________________
struct CColliderRect: public Component {
	/////////////////////////////////
	float w;
	float h;
	/////////////////////////////////



	/////////////////////////////////
	// Constructor for CColliderRect, allowing optional width and height parameters. Defaults to 0.0f for both dimensions.
	CColliderRect(float width = 0.0f, float height = 0.0f) : w(width), h(height) {}
	/////////////////////////////////
};
/////////////////////////////////