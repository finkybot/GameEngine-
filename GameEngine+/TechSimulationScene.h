/////////////////////////////////
// TechSimulationScene class definition, derived from Scene, representing a specific scene in the game engine for simulating technology-related entities and interactions. 
// It includes methods for updating, rendering, handling events, and managing the scene lifecycle, as well as private helper methods for creating a test world and rendering debug information. 
// The class holds references to the render window and tracks frames per second (FPS) for performance monitoring.
/////////////////////////////////



/////////////////////////////////
// Includes and necessary headers for the TechSimulationScene implementation.
#pragma once
#include "Scene.h"
#include "EntityManager.h"
#include "TechRegistry.h"
#include "KnowledgeParticleMovementSystem.h"
#include "TechDiffusionSystem.h"
#include "TechEvolutionSystem.h"
#include "TechUnlockSystem.h"
/////////////////////////////////



/////////////////////////////////
// TechSimulationScene class - A scene for simulating technology-related entities and interactions in the game engine. It provides methods for updating, rendering, handling events, and managing the scene lifecycle, as well as private helper methods for creating a test world and rendering debug information.
//								|
//								|_______________________________________________________________________
class TechSimulationScene : public Scene {
	/////////////////////////////////
	// Public interface for the TechSimulationScene class
public:
	/////////////////////////////////
	// Constructor and destructor for the TechSimulationScene class. The constructor initializes the scene with references to the game engine, render window, and entity manager, while the destructor can be used to clean up any resources if needed.
	TechSimulationScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& em);
	~TechSimulationScene() override;
	/////////////////////////////////



	/////////////////////////////////
	// Overridden virtual methods from the Scene base class. These methods handle updating the scene state, rendering the scene, processing input events, and managing the scene lifecycle (entering and exiting).
	void Update(float dt) override;
	void Render() override;
	void DoAction() override;
	void HandleEvent(const std::optional<sf::Event>& event) override;

	void OnEnter() override;
	void OnExit() override;
	void OnWindowResized(sf::Vector2u newSize) override;

	void LoadResources() override;
	void UnloadResources() override;

	void InitializeGame(sf::Vector2u windowSize) override;
	/////////////////////////////////



	/////////////////////////////////
	// Optional: draw overlays after ImGui/UI has been rendered (engine calls this after ImGui::SFML::Render)
private:
	/////////////////////////////////
	// Private helper methods for the TechSimulationScene class
	void CreateTechTestWorld();
	void RenderTechDebugWindow();
	/////////////////////////////////



	/////////////////////////////////
	// Member variables for the TechSimulationScene class
	sf::RenderWindow& m_window;
	float m_fps = 0.0f;

	KnowledgeParticleMovementSystem m_kpSystem;
	TechDiffusionSystem m_diffusionSystem;
	TechEvolutionSystem m_evolutionSystem;
	TechUnlockSystem m_unlockSystem;
	/////////////////////////////////
};
/////////////////////////////////
