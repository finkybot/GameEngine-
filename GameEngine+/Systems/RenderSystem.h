// ***** RenderSystem.h - Render system for drawing entities *****
#pragma once
#include <vector>
#include <memory>

namespace sf { class RenderWindow; }
class Entity;

// Forward declare FontManager to avoid header cycles
class FontManager;

// RenderSystem is responsible for rendering all alive entities to the SFML render window. It iterates through the list of entities, checks if they are alive, and draws their shapes and text components if present.
class RenderSystem
{
	// ****** Public Methods *****
public:
	RenderSystem() = default;	// Constructor - default is fine since we have no member variables to initialize
	~RenderSystem() = default;	// Destructor - default is fine since we have no resources to clean up

	void RenderAliveEntities(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window); 	// Renders all alive entities to the provided SFML render window (why was you missing????)

	// Convenience render mode for RenderAll
	enum class RenderMode
	{
		ShapesOnly,
		ShapesThenText,
		ShapesThenTextAfterOverlays // Render shapes now, caller should call RenderText() after overlays
	};

	// Convenience: render all entities with a single call. Mode controls whether text is rendered immediately or deferred.
	void RenderAll(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window, RenderMode mode = RenderMode::ShapesThenText);
	
	// Render a single entity (shape and text if present).
	void RenderEntity(Entity* entity, sf::RenderWindow& window) const;	// Helper function that renders a single entity (shape and text if present), called by the public Render method.

	// Render only shapes for all alive entities.
	void RenderShapes(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window);

	// Render only text for all alive entities.
	void RenderText(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window) const;

    // Set the FontManager used for text rendering (optional). If not set, text rendering is skipped.
	void SetFontManager(FontManager* fm) { m_fontManager = fm; }

	// Render text for a single entity using the configured FontManager.
	void RenderTextEntity(Entity* entity, sf::RenderWindow& window) const;

	// ****** Public Accessors ******
	FontManager* GetFontManager() const { return m_fontManager; }

private:
	FontManager* m_fontManager = nullptr; // non-owning pointer to shared FontManager
};
