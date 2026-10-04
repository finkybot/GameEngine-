/////////////////////////////////
// CMouseState.h
/////////////////////////////////


/////////////////////////////////
// Includes
#pragma once
#include "Component.h"
#include "Vec2.h"
/////////////////////////////////



/////////////////////////////////
//	|	CMouseState structure - stores the current state of the mouse, including its screen-space and world-space positions, as well as the state of the left mouse button (down, pressed, released). This component can be attached to an entity to track mouse input and provide relevant information for gameplay 
//	|	or UI interactions.
//	|_______________________________________________________________________
struct CMouseState : public Component {
	Vec2 screenPos; // raw screen-space mouse position
	Vec2 worldPos;	// converted world-space mouse position
	bool leftDown = false;
	bool leftPressed = false;
	bool leftReleased = false;
};
/////////////////////////////////