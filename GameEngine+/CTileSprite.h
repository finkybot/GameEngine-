/////////////////////////////////
// CTileSprite.h
/////////////////////////////////



/////////////////////////////////
// Includes.
#pragma once
#include "Component.h"
#include "Vec2.h"
#include <SFML/Graphics/Color.hpp>
#include <string>	
/////////////////////////////////



/////////////////////////////////
// CTileSprite component -	Represents a tile sprite in the game world, with properties for the texture atlas key, tile index, size, color, and visibility.
//						|
//						|___________________________________________________________________________________
class CTileSprite : public Component {
public:
	/////////////////////////////////
	std::string atlasKey;					// key used with TextureManager
	int tileIndex = 0;						// index into the atlas (0-based)
	float w = 16.f;							// width of the tile sprite
	float h = 16.f;							// height of the tile sprite
	sf::Color color = sf::Color::White;		// color tint of the tile sprite
	bool visible = true;					// visibility flag
	/////////////////////////////////
};
/////////////////////////////////