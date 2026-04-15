// ***** ImageManagementSystem.h - Manages loading and processing of images for the game engine *****
#pragma once
#include <SFML/Graphics.hpp>

using uint = unsigned int; // Alias for unsigned int to simplify code readability

// ImageManagementSystem is responsible for loading images from files and creating tile maps from those images.
// It provides static methods for loading an image into an sf::Image object and for creating a tile map by slicing an image into smaller textures based on specified tile dimensions.
// The class is designed to be used as a utility for managing image assets in the game engine, allowing for efficient loading and processing of images for use in rendering and game logic.
class ImageManagementSystem {
	// ***** Public Methods *****
public:
	static sf::Image LoadImage(
		const std::string&
			filePath); // Loads an image from the specified file path and returns it as an sf::Image object. It takes a string representing the file path of the image to load, attempts to load the image using SFML's loadFromFile method, and returns the loaded image. If loading fails, it logs an error message to the console and returns an empty sf::Image object.
	static std::vector<sf::Texture> CreateTileMap(
		int x, int y, int width, int height,
		const sf::Image&
			image); // Creates a tile map texture by slicing the provided image into smaller textures based on the specified tile dimensions. It takes the x and y coordinates of the top-left corner of the tile, the width and height of each tile, and the source image as parameters. It calculates how many tiles can fit horizontally and vertically in the source image, creates a new texture for each tile by loading a sub-rectangle of the image, and returns a vector of the created textures. If any texture creation fails, it logs an error message and returns false.

private:
	ImageManagementSystem(); // Private constructor to prevent instantiation of this utility class, since all methods are static and it is not meant to be instantiated.
};
using IMS =
	ImageManagementSystem; // Alias for ImageManagementSystem to simplify code readability when calling static methods (e.g., IMS::LoadImage instead of ImageManagementSystem::LoadImage)
