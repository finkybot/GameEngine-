// ***** ImageManagementSystem.h - Manages loading and processing of images for the game engine *****
#pragma once
#include <SFML/Graphics.hpp>

using uint = unsigned int; // Alias for unsigned int to simplify code readability

// ImageManagementSystem is responsible for loading images from files and creating tile maps from those images. 
// It provides static methods for loading an image into an sf::Image object and for creating a tile map by slicing an image into smaller textures based on specified tile dimensions. 
// The class is designed to be used as a utility for managing image assets in the game engine, allowing for efficient loading and processing of images for use in rendering and game logic.
class ImageManagementSystem
{
	// ***** Public Methods *****
public:
		
	static bool LoadImage(const std::string& filePath, sf::Image& image);														// Loads an image from the specified file path into the provided sf::Image object. It takes a file path as a string and a reference to an sf::Image object where the loaded image will be stored. The method returns true if the image was successfully loaded, or false if there was an error (e.g., file not found, unsupported format). It also logs any errors encountered during loading to the console for debugging purposes.
	static bool CreateTileMap(int x, int y, int width, int height, const sf::Image& image, std::vector<sf::Texture>& texture);	// Creates a tile map by slicing the provided sf::Image into smaller textures based on the specified tile dimensions. It takes the starting x and y coordinates, the width and height of each tile, a reference to the source image, and a reference to a vector where the created textures will be stored. The method calculates how many tiles can fit horizontally and vertically in the source image based on the tile dimensions, then iterates through the grid of tiles, creating a new sf::Texture for each tile by loading it from the corresponding sub-rectangle of the source image. If any texture fails to load, it logs an error message to the console and returns false; otherwise, it returns true after successfully creating all tile textures.
		
	// ***** Private Methods *****
private:
	ImageManagementSystem();	// Private constructor to prevent instantiation of this utility class, since all methods are static and it is not meant to be instantiated.

}; using IMS = ImageManagementSystem; // Alias for ImageManagementSystem to simplify code readability when calling static methods (e.g., IMS::LoadImage instead of ImageManagementSystem::LoadImage)

