/////////////////////////////////
// CRenderInstance.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
/////////////////////////////////



/////////////////////////////////
//	| CRenderInstance struct - defines the properties of a render instance, including its position, rotation, scale, color, and texture ID. This struct is used to store the characteristics of individual renderable objects in the game.
//	|_______________________________________________________________________
struct CRenderInstance : public Component {
	float radius; // The radius of the render instance, used for rendering and collision detection.
	float r, g, b, a; // The color of the render instance, represented as RGBA values for rendering purposes.
};
/////////////////////////////////