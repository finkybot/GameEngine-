/////////////////////////////////
//  ImageManagementSystem.h
/////////////////////////////////



/////////////////////////////////
// Includes 
#pragma once
#include <SFML/Graphics.hpp>
/////////////////////////////////



/////////////////////////////////
// Type alias for unsigned int to simplify code readability when working with tile coordinates and dimensions, since these values are typically non-negative and we want to avoid confusion with signed integers.
using uint = unsigned int;
/////////////////////////////////



/////////////////////////////////
//	|	ImageManagementSystem class - A utility class for managing image assets in the game engine. It provides static methods for loading images and creating tile maps from those images.
//	|	The class is designed to be a utility class and does not need to be instantiated, so all methods are static. It uses the SFML library for image loading and manipulation, allowing for easy integration with the rest of the game engine.		
//	|_______________________________________________________________________
class ImageManagementSystem {
public:
	// LoadImage method - Loads an image from the specified file path and returns it as an sf::Image object. It takes a string representing the file path of the image to load, attempts to load the image using SFML's loadFromFile method, and returns the loaded image. If loading fails, it logs an error message 
	// to the console and returns an empty sf::Image object.
	static sf::Image LoadImage(const std::string& filePath);


	// CreateTileMap method - Creates a tile map texture by slicing the provided image into smaller textures based on the specified tile dimensions. It takes the x and y coordinates of the top-left corner of the tile, the width and height of each tile, and the source image as parameters.
	static std::vector<sf::Texture> CreateTileMap(int x, int y, int width, int height,	const sf::Image& image);


	// Private member variables, methods and constructor for the ImageManagementSystem class.
private:
	// Private constructor to prevent instantiation of the ImageManagementSystem class, since it is intended to be a utility class with only static methods. This ensures that no instances of the class can be created, and all functionality is accessed through the static methods.
	ImageManagementSystem(); 
};
////////////////////////////////



// Type alias for ImageManagementSystem to simplify code readability when calling static methods. This allows us to use IMS::LoadImage instead of ImageManagementSystem::LoadImage, making the code cleaner and easier to read when working with image loading and tile map creation.
using IMS =	ImageManagementSystem;
