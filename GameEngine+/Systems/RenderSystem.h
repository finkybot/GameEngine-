/////////////////////////////////
// RenderSystem.h - Render system for drawing entities
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <vector>
#include <memory>
/////////////////////////////////



/////////////////////////////////
// Forward declarations to avoid header cycles. We forward declare the SFML RenderWindow class and the Entity class, as well as the FontManager class which is used for text rendering. This allows us to use pointers or references to these types without 
// including their full definitions in this header, which can help reduce compilation dependencies and improve build times.
namespace sf {
	class RenderWindow;
}

class Entity;
class FontManager;
/////////////////////////////////



/////////////////////////////////
// RenderSystem is responsible for rendering all alive entities to the SFML render window. It iterates through the list of entities, checks if they are alive, and draws their shapes and text components if present.
class RenderSystem {
	/////////////////////////////////
	// Public interface
public:
	/////////////////////////////////
	// Constructor and destructor for the RenderSystem class. The default constructor and destructor are sufficient since we don't have any member variables that require special initialization or cleanup. 
	// If we add member variables in the future that require custom handling, we can implement these methods accordingly.
	RenderSystem() = default; 
	~RenderSystem() = default;
	/////////////////////////////////



	/////////////////////////////////
	// RenderAliveEntities - Renders all alive entities to the provided SFML render window. This method iterates through the list of entities, checks if they are alive, and draws their shapes and text components 
	// if present. It serves as the main entry point for rendering entities in the game loop.
	void RenderAliveEntities(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window);
	/////////////////////////////////



	/////////////////////////////////
	// RenderMode enum - Defines different rendering modes for the RenderAll method. This enum allows the caller to specify whether to render only shapes, render shapes followed by text, or render shapes now and defer text rendering until after overlays are rendered. 
	enum class RenderMode {
		ShapesOnly,
		ShapesThenText,
		ShapesThenTextAfterOverlays // Render shapes now, caller should call RenderText() after overlays
	};
	/////////////////////////////////



	/////////////////////////////////
	// RenderAll - A convenience method that renders all entities with a single call. The mode parameter controls whether to render only shapes, render shapes followed by text, or render shapes now and defer text rendering until after overlays are rendered.
	void RenderAll(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window, RenderMode mode = RenderMode::ShapesThenText);
	/////////////////////////////////



	/////////////////////////////////
	// RenderEntity - Renders a single entity (shape and text if present).
	void RenderEntity(Entity* entity, sf::RenderWindow& window) const;
	/////////////////////////////////



	/////////////////////////////////
	// RenderShapes - Render only shapes for all alive entities.
	void RenderShapes(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window);
	/////////////////////////////////



	/////////////////////////////////
	// RenderText - Render only text for all alive entities.
	void RenderText(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window) const;
	/////////////////////////////////



	/////////////////////////////////
	// SetFontManager - Set the FontManager used for text rendering (optional). If not set, text rendering is skipped.
	void SetFontManager(FontManager* fm) { m_fontManager = fm; }
	/////////////////////////////////



	/////////////////////////////////
	// RenderTextEntity - Render text for a single entity using the configured FontManager.
	void RenderTextEntity(Entity* entity, sf::RenderWindow& window) const;
	/////////////////////////////////



	/////////////////////////////////
	// GetFontManager - Get the current FontManager used for text rendering.
	FontManager* GetFontManager() const { return m_fontManager; }
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the RenderSystem class.
private:
	/////////////////////////////////
	// Non-owning pointer to a shared FontManager for text rendering. This allows the RenderSystem to use a common FontManager instance that is managed elsewhere in the engine, without taking ownership of it. If this pointer is not set, text rendering will be skipped.
	FontManager* m_fontManager = nullptr;
	/////////////////////////////////
};
/////////////////////////////////
