// ImageManagementSystem.cpp
#include "ImageManagementSystem.h"
#include <iostream>
#include <SFML/Graphics/Rect.hpp>
#include <cmath>

bool ImageManagementSystem::LoadImage(const std::string& filePath, sf::Image& image)
{
    try {
        image.loadFromFile(filePath);
        return true;
    }
    catch (const std::exception& e) {
        // Log the error message 
        std::cerr << "Error loading image from " << filePath << ": " << e.what() << std::endl;
	}
    return false;
}

bool ImageManagementSystem::CreateTileMap(int x, int y, int width, int height, const sf::Image& image, std::vector<sf::Texture>& textures)
{
	int xnumTiles = std::floor(image.getSize().x / (width));
	int ynumTiles = std::floor(image.getSize().y / (height));
    
	int padding = 0; // Adjust if there is padding between tiles in the source image
    for (int row = 0; row < ynumTiles; ++row)
    {
        for (int col = 0; col < xnumTiles; ++col)
        {
            sf::Texture texture;
            if (texture.loadFromImage(image, false, sf::IntRect({ col * (width), row * (height)}, { width, height })))
            {
                textures.push_back(std::move(texture));
            }
            else
            {
                std::cerr << "Failed to create tile map texture for tile at (" << col << ", " << row << ")." << std::endl;
                return false;
            }
			padding = 1; // Set padding to 1 after the first tile to account for any spacing between tiles in the source image
        }
	}
    return true;
}
