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
#include <atomic>
#include <cstdint>
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
	void RunFullTechTick();
	/////////////////////////////////



	/////////////////////////////////
	// Job scheduling methods for the TechSimulationScene class	
	void ScheduleTechEvolutionJobs(float dt);
	/////////////////////////////////



	/////////////////////////////////
	// Job scheduling methods for the TechSimulationScene class
	void ScheduleTechDiffusionJobs(float dt);
	/////////////////////////////////



	/////////////////////////////////
	// Member variables for the TechSimulationScene class
	sf::RenderWindow& m_window;
	float m_fps = 0.0f;
	/////////////////////////////////



	/////////////////////////////////
	// Systems for managing different aspects of the technology simulation in the TechSimulationScene class
	KnowledgeParticleMovementSystem m_kpSystem;
	TechDiffusionSystem m_diffusionSystem;
	TechEvolutionSystem m_evolutionSystem;
	TechUnlockSystem m_unlockSystem;
	/////////////////////////////////



	/////////////////////////////////
	// Scene state variables for the TechSimulationScene class
	std::atomic<bool> m_isActive	{ true };		// Flag to indicate whether the scene is currently active and should be updated/rendered
	std::atomic<uint64_t> m_jobGeneration{ 0 }; // Monotonic token used to cancel stale scheduled jobs across scene transitions
	size_t m_civBudget = 200;					// process only 200 civs per frame
	size_t m_lastCivIndex = 0;					// index of the last civ processed in the previous frame
	std::chrono::steady_clock::time_point m_sceneStartTime; // Time point marking the start of the scene, used for tracking elapsed time
	/////////////////////////////////



	/////////////////////////////////
	// Timers for scheduling jobs in the TechSimulationScene class
	float m_diffusionTimer = 0.0f; // Timer for scheduling diffusion jobs
	float m_evolutionTimer = 0.0f; // Timer for scheduling evolution jobs
	/////////////////////////////////



	/////////////////////////////////
	// Intervals for scheduling jobs in the TechSimulationScene class
	//const float m_diffusionInterval = 0.25f; // Interval for scheduling diffusion jobs
	//const float m_evolutionInterval = 0.5f; // Interval for scheduling evolution jobs
	/////////////////////////////////



	/////////////////////////////////
	float m_techAccumulator = 0.0f;
	const float m_techInterval = 1.0f;
	/////////////////////////////////
};
/////////////////////////////////
