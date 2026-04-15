// ******** FontManager.h ********
#pragma once
#include <optional>
#include <memory>
#include <mutex>
#include <SFML/Graphics/Font.hpp>
#include <String>
#include <unordered_map>

class FontManager {
public:
	// Constructors ~ Destructors.
	FontManager() = default;
	~FontManager() = default;

	// Public Methods. ******
	// Retrieves a font by name. If the font is not already loaded, it will attempt to load it from the specified file path. If the font is successfully loaded,
	// it will be stored in the manager for future retrieval. If the font cannot be loaded, an empty optional will be returned.
	bool LoadFont(const std::string& name, const std::string& filePath);

	// Retrieves a font by name. If the font is already loaded, it will return a shared pointer to the font. If the font is not found, it will return an empty optional.
	std::optional<std::shared_ptr<sf::Font>> GetFont(const std::string& name) const;

	// Unloads a font by name. If the font is currently loaded, it will be removed from the manager and its resources will be freed. If the font is not found, this method will do nothing.
	void UnloadFont(const std::string& name);

	// For Later.... Consider looking at SDF fonts for better performance and quality at various sizes. This would involve generating signed distance field textures for the fonts and using a
	// custom shader to render them, which can provide better visual quality and performance when rendering text at different sizes. It would be a more complex implementation but could be worth it
	// for a game engine that needs to render a lot of text efficiently. (<--- err yea!!! What he/she/it/undefined (A.I) said)

private:
	mutable std::mutex m_mutex;
	std::unordered_map<std::string, std::shared_ptr<sf::Font>> m_fonts;
};
