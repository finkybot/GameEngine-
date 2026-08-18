/////////////////////////////////
// CLayer.h
/////////////////////////////////



/////////////////////////////////
// Includes and necessary headers for the CLayer component.
#pragma once
#include "Component.h"
/////////////////////////////////



/////////////////////////////////
// CLayer component - represents the rendering layer of an entity, allowing for control over the rendering order of entities in the game. This component can be used by the RenderSystem to determine 
// the order in which entities are drawn on the screen, with different layers representing different depths in the scene (e.g., background, midground, foreground, overlay).
//								|
//								|_______________________________________________________________________
class CLayer : public Component {
	/////////////////////////////////
	// Public data members for CLayer.
public:
	/////////////////////////////////
	// Layer enumeration to define different rendering layers for entities. The layers are defined in a specific order, with Background being the furthest back layer and Overlay being the topmost layer.
    enum class Layer { Background = 0, Mid = 1, Foreground = 2, Overlay = 3 };
    Layer m_layer = Layer::Mid;
	/////////////////////////////////



	/////////////////////////////////
	// Constructors for the CLayer component. The default constructor initializes the layer to the default Mid layer, while the constructor with a Layer parameter allows for specifying a specific rendering layer for the entity.
    CLayer() = default;
    explicit CLayer(Layer layer) : m_layer(layer) {}
	/////////////////////////////////
};
/////////////////////////////////
