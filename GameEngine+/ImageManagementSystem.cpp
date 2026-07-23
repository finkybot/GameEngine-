/////////////////////////////////
// ImageManagementSystem.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "ImageManagementSystem.h"
#include <iostream>
#include <SFML/Graphics/Rect.hpp>
#include <cmath>
/////////////////////////////////



/////////////////////////////////
// LoadImage - Loads an image from the specified file path and returns it as an sf::Image object. It takes a string representing 
// the file path of the image to load, attempts to load the image using SFML's loadFromFile method, and returns the loaded image. 
// If loading fails, it logs an error message to the console and returns an empty sf::Image object.
sf::Image ImageManagementSystem::LoadImage(const std::string& filePath) {
	try {
		sf::Image image;
		if (image.loadFromFile(filePath)) {
			return image;
		}
	} catch (const std::exception& e) {
		// Log the error message
		std::cerr << "Error loading image from " << filePath << ": " << e.what() << std::endl;
	}
	return sf::Image();
}
/////////////////////////////////



/////////////////////////////////
// CreateTileMap - Creates a tile map texture by slicing the provided image into smaller textures based on the specified tile dimensions. 
// It takes the x and y coordinates of the top-left corner of the tile, the width and height of each tile, and the source image as parameters.
std::vector<sf::Texture> ImageManagementSystem::CreateTileMap(int x, int y, int width, int height,
															  const sf::Image& image) {
	std::vector<sf::Texture> textures;
	int xnumTiles = std::floor(image.getSize().x / (width));
	int ynumTiles = std::floor(image.getSize().y / (height));

	int padding = 0; // Adjust if there is padding between tiles in the source image
	for (int row = 0; row < ynumTiles; ++row) {
		for (int col = 0; col < xnumTiles; ++col) {
			sf::Texture texture;
			if (texture.loadFromImage(image, false, sf::IntRect({col * (width), row * (height)}, {width, height}))) {
				textures.push_back(std::move(texture));
			} else {
				std::cerr << "Failed to create tile map texture for tile at (" << col << ", " << row << ")."
						  << std::endl;
				return textures;
			}
			padding =
				1; // Set padding to 1 after the first tile to account for any spacing between tiles in the source image
		}
	}
	std::cout << "Created tile map with " << textures.size() << " tiles (" << xnumTiles << "x" << ynumTiles << ")."
			  << std::endl;
	return textures;
}
/////////////////////////////////
