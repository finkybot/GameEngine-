// FontManager.cpp
#include "FontManager.h"

#include <filesystem>
#include <iostream>

// Implementation of FontManager methods. This class is responsible for loading, retrieving, and unloading fonts in a thread-safe manner using a mutex to protect access to the internal font map.

// Loads a font from the specified file path and associates it with the given name. If the font is successfully loaded,
// it will be stored in the manager for future retrieval. If the font cannot be loaded, an empty optional will be returned.
// Note:    Making the method thread-safe by locking the mutex when accessing the font map.
// Note:    SFML 3.0 changed the way fonts are loaded, so we need to create a shared pointer to a font and call openFromFile (loadFromFile in previous versions) on it to load the font data.
//          If the font loads successfully, we store it in the map and return true. If it fails to load, we return false.
bool FontManager::LoadFont(const std::string& name, const std::string& filePath) {
	// Lock the mutex to ensure thread safety when accessing the font map.
	std::lock_guard<std::mutex> lock(m_mutex);

	// Dealing with a couple of guards here, to ensure we don't accidentally overwrite existing fonts or attempt to load a font from a non-existent file path.
	// The code is similar to the texture loading code in TextureManager, but adapted for fonts and the changes in SFML 3.0's font loading mechanism. (A.I gets this spot on, by all those computing jobs)
	// First, we check if a font with the same name already exists in the map. If it does, we return false to indicate that the font cannot be loaded because it would overwrite an existing font.
	// Then we check if the font file exists at the specified path before attempting to load it. If the file does not exist, we return false to indicate that the font cannot be loaded because the file is missing.
	if (m_fonts.find(name) != m_fonts.end()) {
		return false;
	}
	if (!std::filesystem::exists(filePath)) {
		std::cerr << "Font file not found: " << filePath << std::endl;
		return false;
	}

	// With guards in place, we can now attempt to load the font. We create a shared pointer to a font and call openFromFile on it to load the font data from the specified file path.
	// If the font loads successfully, we store it in the map and return true. If it fails to load, we return false to indicate failure.
	if (auto font = std::make_shared<sf::Font>(); font->openFromFile(filePath)) {
		m_fonts[name] = font;
		return true;
	}

	// If we get here then the font has failed to load, return false to indicate failure.
	std::cerr << "Failed to load font: " << filePath << std::endl;
	return false;
}

// Retrieves a font by name. If the font is already loaded, it will return a shared pointer to the font. If the font is not found, it will return an empty optional.
// Note:    Making the method thread-safe by locking the mutex when accessing the font map.
std::optional<std::shared_ptr<sf::Font>> FontManager::GetFont(const std::string& name) const {
	// Lock the mutex to ensure thread safety when accessing the font map.
	std::lock_guard<std::mutex> lock(m_mutex);

	// Look up the font by name in the map. If it is found, return a shared pointer to the font. If it is not found, return an empty optional. SIMPEEEELLLLSSS
	auto it = m_fonts.find(name);
	if (it != m_fonts.end()) {
		return it->second; // return the (value) shared pointer to the font if found
	}
	return std::nullopt; // I like these optionals
}

// Unloads a font by name. If the font is currently loaded, it will be removed from the manager and its resources will be freed. If the font is not found, this method will do nothing.
// Note:    Guess what??? Making the method thread-safe by locking the mutex when accessing the font map. We simply erase the font from the map if it exists,
//			which will free its resources if there are no other shared pointers referencing it.
void FontManager::UnloadFont(const std::string& name) {
	// Lock the mutex to ensure thread safety when accessing the font map.
	std::lock_guard<std::mutex> lock(m_mutex);

	// Erase the font from the map if it exists. If the font is not found, this will do nothing.
	m_fonts.erase(name);
}
