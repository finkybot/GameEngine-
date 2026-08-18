/////////////////////////////////
// FontManager.h
/////////////////////////////////



/////////////////////////////////
// The FontManager class is responsible for loading, storing, and retrieving fonts in the game engine. It provides an interface for managing font resources, allowing for efficient loading and reuse of fonts throughout the game. 
// The FontManager uses a map to store loaded fonts by name, and it provides methods for loading fonts from files, retrieving fonts by name, and unloading fonts when they are no longer needed. 
// This helps to optimize memory usage and improve performance by avoiding redundant loading of the same font multiple times.
#pragma once
#include <optional>
#include <memory>
#include <mutex>
#include <SFML/Graphics/Font.hpp>
#include <String>
#include <unordered_map>
/////////////////////////////////



/////////////////////////////////
// FontManager class declaration. This class manages the loading, storage, and retrieval of fonts in the game engine, providing an interface for efficient font resource management.
//								|
//								|_______________________________________________________________________
class FontManager {
	/////////////////////////////////
	// Public interface for the FontManager class.
public:
	/////////////////////////////////
	// Constructor and destructor for the FontManager class. The constructor initializes the font manager, while the destructor ensures that any loaded fonts are properly cleaned up when the manager is destroyed.
	FontManager() = default;
	~FontManager() = default;
	/////////////////////////////////



	/////////////////////////////////
	// LoadFont - loads a font from a specified file path and associates it with a given name. If the font is successfully loaded, it will be stored in the manager and can be retrieved later using the name. 
	// If the font fails to load (e.g., due to an invalid file path or unsupported format), this method will return false, indicating that the loading was unsuccessful.
	bool LoadFont(const std::string& name, const std::string& filePath);
	/////////////////////////////////
	 
	

	/////////////////////////////////
	// GetFont - retrieves a font by name. If the font is already loaded, it will return a shared pointer to the font. If the font is not found, it will return an empty optional, allowing the caller to handle the case where the requested font is not available.
	std::optional<std::shared_ptr<sf::Font>> GetFont(const std::string& name) const;
	/////////////////////////////////



	/////////////////////////////////
	// UnloadFont - unloads a font by name. If the font is currently loaded, it will be removed from the manager and its resources will be freed. If the font is not found, this method will do nothing.
	void UnloadFont(const std::string& name);
	/////////////////////////////////


	/////////////////////////////////
	// For Later.... Consider looking at SDF fonts for better performance and quality at various sizes. This would involve generating signed distance field textures for the fonts and using a
	// custom shader to render them, which can provide better visual quality and performance when rendering text at different sizes. It would be a more complex implementation but could be worth it
	// for a game engine that needs to render a lot of text efficiently. (<--- err yea!!! What he/she/it/undefined (A.I) said)
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the FontManager class
private:
	/////////////////////////////////
	// Mutex to protect access to the fonts map across threads, allowing for safe concurrent loading and retrieval; the map itself stores shared pointers to sf::Font objects, keyed by their associated names for easy retrieval.
	mutable std::mutex m_mutex;
	std::unordered_map<std::string, std::shared_ptr<sf::Font>> m_fonts;
	/////////////////////////////////
};
/////////////////////////////////
