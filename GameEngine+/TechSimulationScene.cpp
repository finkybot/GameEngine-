/////////////////////////////////
// TechSimulationScene.cpp - Implementation of the TechSimulationScene class, which is a derived class of Scene. This class manages the technology simulation scene, including updating the scene state, rendering, handling input events, and managing the scene lifecycle. It also includes methods for creating a test 
// world with civilizations and knowledge particles, as well as rendering a debug window for displaying technology information.
/////////////////////////////////



/////////////////////////////////
// Includes
#include "TechSimulationScene.h"
#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>
#include <imgui/imgui_internal.h>
#include <Windows.h>
#include <string>

#include "CTransform.h"
#include "CCivilisationTech.h"
#include "CKnowledgeParticle.h"
#include "CParticleInfluence.h"
#include "JobSystem.h"
/////////////////////////////////



/////////////////////////////////
// Helper function to get a color based on progress value (t) for rendering purposes. The function returns a color that transitions from red to yellow to green as the progress value increases from 0 to 1.
static ImVec4 GetProgressColor(float t) {
	// t ∈ [0,1]
	if (t < 0.33f) {
		return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // red
	} else if (t < 0.66f) {
		return ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // yellow
	} else {
		return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // green
	}
}
/////////////////////////////////



/////////////////////////////////
// Implementation of the TechSimulationScene class
TechSimulationScene::TechSimulationScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& em): Scene(engine, em), m_window(win), m_kpSystem(), m_diffusionSystem(engine.techRegistry),
																										m_evolutionSystem(engine.techRegistry), m_unlockSystem(engine.techRegistry) {
	ImGui::SFML::Init(engine.window);
}
/////////////////////////////////



/////////////////////////////////
// Destructor for the TechSimulationScene class
TechSimulationScene::~TechSimulationScene() = default;
/////////////////////////////////



/////////////////////////////////
// OnEnter - Called when the scene is entered. Initializes the game with the current window size.
void TechSimulationScene::OnEnter() {
	// Advance generation so any stale jobs from a previous lifetime self-cancel.
	m_jobGeneration.fetch_add(1, std::memory_order_acq_rel);
	m_isActive.store(true, std::memory_order_release);
	JobSystem::WaitIdle(); // ensure all jobs finish before continuing
	m_entityManager.ClearAll();  
	InitializeGame(m_gameEngine.windowSize);

	// Reset timers for diffusion and evolution systems
	m_sceneStartTime = std::chrono::steady_clock::now();
}
/////////////////////////////////



/////////////////////////////////
// OnExit - Called when the scene is exited. Currently does nothing, but can be used for cleanup if needed.
void TechSimulationScene::OnExit() {
	m_isActive.store(false, std::memory_order_release);
	// Invalidate queued jobs immediately so stale lambdas exit deterministically.
	m_jobGeneration.fetch_add(1, std::memory_order_acq_rel);
	JobSystem::WaitIdle(); // ensure all jobs finish before continuing
}
/////////////////////////////////



/////////////////////////////////
// LoadResources - Loads the necessary resources for the scene.
void TechSimulationScene::LoadResources() {
	m_isLoaded = true;
}
/////////////////////////////////



/////////////////////////////////
// UnloadResources - Unloads the resources for the scene. Currently does nothing, but can be used for cleanup if needed.
void TechSimulationScene::UnloadResources() {}
/////////////////////////////////



/////////////////////////////////
// InitializeGame - Initializes the game state for the scene. Creates a test world with civilizations and knowledge particles.
void TechSimulationScene::InitializeGame(sf::Vector2u windowSize) {
	CreateTechTestWorld();
}
/////////////////////////////////



/////////////////////////////////
// CreateTechTestWorld - Creates a test world with two civilizations and a knowledge particle. Civilization A knows the "agriculture.basic
void TechSimulationScene::CreateTechTestWorld() {
	// Spawn many civilisations
	const int civCount = 1550;
	for (int i = 0; i < civCount; i++) {
		Entity* civ = m_entityManager.AddEntity(EntityType::Civilisation);

		auto* t = civ->AddComponent<CTransform>();
		t->position = Vec2(rand() % 2000, rand() % 2000);

		auto* tech = civ->AddComponent<CCivilisationTech>();

		if (i == 0) {
			// Civ 0 knows the tech
			tech->knownTechs["agriculture.basic"] = 1.0f;
			tech->unlockedTechs.insert("agriculture.basic");
		} else {
			tech->knownTechs["agriculture.basic"] = 0.0f;
			tech->passiveProgress["agriculture.basic"] = 0.0f;
		}
	}

	// Spawn many knowledge particles
	const int particleCount = 60;
	for (int i = 0; i < particleCount; i++) {
		Entity* p = m_entityManager.AddEntity(EntityType::KnowledgeParticle);

		auto* kp = p->AddComponent<CKnowledgeParticle>();
		kp->techId = "agriculture.basic";
		kp->value = 1.0f;

		auto* t = p->AddComponent<CTransform>();
		t->position = Vec2(rand() % 2000, rand() % 2000);
		t->velocity = Vec2(0, 0);

		auto* inf = p->AddComponent<CParticleInfluence>();
		inf->influenceRadius = 500.0f;
		inf->influenceFalloff = 1.0f;
	}
}
/////////////////////////////////



/////////////////////////////////
// Update - Updates the scene state, running the technology systems and rendering the debug window.
void TechSimulationScene::Update(float dt) {
	//frameCounter++; // Increment frame counter

	// Update FPS and adjust civBudget based on performance
	float fps = 1.0f / dt;

	// Adjust civBudget based on FPS to maintain performance
	if (fps < 30)
		m_civBudget = 100;
	else if (fps < 45)
		m_civBudget = 150;
	else
		m_civBudget = 200;

	// --- PARALLEL TECH SYSTEMS ---
	// Schedule diffusion and evolution jobs based on their respective intervals, this is frame agnostic and will run jobs at the specified intervals regardless of frame rate.
	m_diffusionTimer += dt;
	m_evolutionTimer += dt;
	
	if (m_diffusionTimer >= m_diffusionInterval) {
		m_diffusionTimer = 0.0f;
		ScheduleTechDiffusionJobs(dt);
	}

	if (m_evolutionTimer >= m_evolutionInterval) {
		m_evolutionTimer = 0.0f;
		ScheduleTechEvolutionJobs(dt);
	}

	// Wait for all scheduled jobs to complete before proceeding to the next frame. This ensures that all technology systems have finished processing before the next update cycle begins.
	JobSystem::WaitIdle(); 

	// One-frame telemetry: report if defensive SEH was used in diffusion.
	const uint32_t sehCaughtThisFrame = m_diffusionSystem.ConsumeSehCatchCount();
	if (sehCaughtThisFrame > 0) {
		std::string msg = "[TechDiffusion] SEH stale-pointer catches this frame: " + std::to_string(sehCaughtThisFrame) + "\n";
		::OutputDebugStringA(msg.c_str());
	}

	// --- SERIAL SYSTEMS (must stay on main thread) ---
	m_kpSystem.Update(dt, m_entityManager);		// movement → mutates transforms
	m_unlockSystem.Update(dt, m_entityManager); // unlocks → triggers events

	// --- DEBUG UI ---
	RenderTechDebugWindow();
}
/////////////////////////////////



/////////////////////////////////
// Render - Renders the scene. Currently does nothing, but can be used for rendering if needed.
void TechSimulationScene::Render() {}
/////////////////////////////////



/////////////////////////////////
// DoAction - Performs an action in the scene. Currently does nothing, but can be used for scene-specific actions if needed.
void TechSimulationScene::DoAction() {}
/////////////////////////////////



/////////////////////////////////
// HandleEvent - Handles input events for the scene, passing them to ImGui for processing.
void TechSimulationScene::HandleEvent(const std::optional<sf::Event>& event) {
	ImGui::SFML::ProcessEvent(m_gameEngine.window, *event);
}
/////////////////////////////////



/////////////////////////////////
// OnWindowResized - Handles window resize events, adjusting the view to match the new window size.
void TechSimulationScene::OnWindowResized(sf::Vector2u newSize) {
	sf::View view;
	view.setCenter(sf::Vector2f(newSize.x * 0.5f, newSize.y * 0.5f));
	view.setSize(sf::Vector2f(newSize.x, newSize.y));
	m_window.setView(view);
}
/////////////////////////////////



/////////////////////////////////
// RenderTechDebugWindow - Renders a debug window using ImGui to display information about the civilizations and their technology progress.
void TechSimulationScene::RenderTechDebugWindow() {
	ImGui::Begin("Tech Simulation Debug");

	// Create two columns: left = civ list, right = graphs
	ImGui::Columns(2, "techColumns", true);

	// ---------------------------
	// LEFT COLUMN (scrollable civ list)
	// ---------------------------
	ImGui::BeginChild("CivList", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

	auto& entities = m_entityManager.GetEntities();

	for (auto& e : entities) {
		Entity* ent = e.get();
		if (!ent || !ent->IsAlive())
			continue;

		auto* tech = ent->GetComponent<CCivilisationTech>();
		if (!tech)
			continue;

		ImGui::Text("Civ %p", ent);

		const auto knownIt = tech->knownTechs.find("agriculture.basic");
		const float known = (knownIt != tech->knownTechs.end()) ? knownIt->second : 0.0f;

		const auto passiveIt = tech->passiveProgress.find("agriculture.basic");
		const float passive = (passiveIt != tech->passiveProgress.end()) ? passiveIt->second : 0.0f;

		const auto activeIt = tech->activeResearch.find("agriculture.basic");
		const bool isActive = (activeIt != tech->activeResearch.end());
		const float activeProgress = isActive ? activeIt->second : 0.0f;

		const CTechNode* node = m_gameEngine.techRegistry.GetTechNode("agriculture.basic");
		const float requiredKnowledge = node ? node->requiredKnowledge : 0.0f;

		// Known progress bar (0–1)
		{
			float t = known; // already normalized
			ImVec4 col = GetProgressColor(t);
			ImGui::BulletText("agriculture.basic known: %.2f", known);
			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
			ImGui::ProgressBar(t, ImVec2(200, 12));
			ImGui::PopStyleColor();
		}

		// Passive progress bar
		{
			float t = requiredKnowledge > 0 ? passive / requiredKnowledge : 0.0f;
			ImVec4 col = GetProgressColor(t);
			ImGui::BulletText("passive: %.2f", passive);
			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
			ImGui::ProgressBar(t, ImVec2(200, 12));
			ImGui::PopStyleColor();
		}

		// Active progress bar
		{
			float t = requiredKnowledge > 0 ? activeProgress / requiredKnowledge : 0.0f;
			ImVec4 col = GetProgressColor(t);
			ImGui::BulletText("active: %s", isActive ? "yes" : "no");
			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
			ImGui::ProgressBar(t, ImVec2(200, 12));
			ImGui::PopStyleColor();
		}


		ImGui::Separator();
	}

	ImGui::EndChild();

	// ---------------------------
	// RIGHT COLUMN (fixed graphs)
	// ---------------------------
	ImGui::NextColumn();

	ImGui::BeginChild("GraphsPanel", ImVec2(0, 0), true);

	// Worker threads
	ImGui::Text("Worker Threads: %zu", JobSystem::GetWorkerCount());
	ImGui::Text("Last Frame Job Time: %.3f ms", JobSystem::GetLastFrameJobTimeMs());

	// Job time graph
	static float history[120] = {0};
	static int index = 0;
	history[index] = JobSystem::GetLastFrameJobTimeMs();
	index = (index + 1) % 120;
	ImGui::PlotLines("Job Time (ms)", history, 120, 0, nullptr, 0.0f, 20.0f, ImVec2(250, 80));

	// FPS
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

	// FPS graph
	static float fpsHistory[120] = {0};
	static int fpsIndex = 0;
	fpsHistory[fpsIndex] = ImGui::GetIO().Framerate;
	fpsIndex = (fpsIndex + 1) % 120;
	ImGui::PlotLines("FPS", fpsHistory, 120, 0, nullptr, 0.0f, 200.0f, ImVec2(250, 80));

	ImGui::Text("Civs with completed tech: %zu", m_evolutionSystem.GetTotalTechCompleted());

	auto elapsed = std::chrono::steady_clock::now() - m_sceneStartTime;
	double seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
	int minutes = static_cast<int>(seconds / 60);
	int secs = static_cast<int>((int)seconds % 60);
	ImGui::Text("Scene runtime: %d:%02d", minutes, secs);

	ImGui::EndChild();

	ImGui::End();
}
/////////////////////////////////
 
 

/////////////////////////////////
// ScheduleTechEvolutionJobs - Schedules technology evolution jobs based on the elapsed time (dt). Currently does nothing, but can be used for scheduling evolution tasks if needed.
void TechSimulationScene::ScheduleTechEvolutionJobs(float dt) {
	// Get the list of entities from the entity manager
	auto& entities = m_entityManager.GetEntities();

	// Define the chunk size for processing entities in parallel. This determines how many entities will be processed in each job.
	const size_t chunkSize = 128;

	// Limit the number of civilizations processed per frame to avoid overloading the system. This is a safeguard to ensure that only a manageable number of civilizations are processed in each update cycle.
	size_t limit = std::min(entities.size(), m_civBudget);


	// Compute rolling window
	size_t startIndex = m_lastCivIndex;
	size_t endIndex = std::min(startIndex + limit, entities.size());

	// Update the last processed index for the next frame, wrapping around if necessary
    for (size_t i = startIndex; i < endIndex; i += chunkSize) {
		size_t begin = i;
		size_t end = std::min(endIndex, i + chunkSize);

		// Check if the scene is active before scheduling jobs
		if (!m_isActive) return;
		
		// Schedule a job to process a chunk of entities for technology evolution
		const uint64_t generation = m_jobGeneration.load(std::memory_order_acquire);
		
		// Schedule a job to process a chunk of entities for technology evolution
		JobSystem::Schedule([this, begin, end, dt, generation]() {
			// Check if the scene is active before processing jobs, yes, this is a double-check, but it is necessary to ensure that the scene is still active when the job runs
			if (!m_isActive.load(std::memory_order_acquire) || generation != m_jobGeneration.load(std::memory_order_acquire)) return; 

			// Get the list of entities from the entity manager
			auto& ents = m_entityManager.GetEntities();

			// Process each entity in the assigned chunk for technology evolution
			for (size_t j = begin; j < end; ++j) {
				Entity* e = ents[j].get();

				// Check if the entity is valid and alive before processing it
				if (!e || !e->IsAlive()) continue;

				// Get the CCivilisationTech component for the entity
				auto* tech = e->GetComponent<CCivilisationTech>();

				// Check if the entity has a CCivilisationTech component
				if (!tech) continue;

				// Process technology evolution for the civilization using the TechEvolutionSystem
				m_evolutionSystem.ProcessCivilisationTech(e, tech, m_entityManager, dt);
			}
		});
	}
}
/////////////////////////////////



/////////////////////////////////
// ScheduleTechDiffusionJobs - Schedules technology diffusion jobs based on the elapsed time (dt). Currently does nothing, but can be used for scheduling diffusion tasks if needed.
void TechSimulationScene::ScheduleTechDiffusionJobs(float dt) {
	// Get the list of entities from the entity manager
	auto& entities = m_entityManager.GetEntities();

	// Define the chunk size for processing entities in parallel. This determines how many entities will be processed in each job.
	const size_t chunkSize = 128;

	// Limit work per frame
	size_t limit = std::min(entities.size(), m_civBudget);

	// Compute rolling window
	size_t startIndex = m_lastCivIndex;
	size_t endIndex = std::min(startIndex + limit, entities.size());

	// Schedule jobs for each chunk of entities
	for (size_t i = startIndex; i < endIndex; i += chunkSize) {
		size_t begin = i;
		size_t end = std::min(endIndex, i + chunkSize);

		// Check if the scene is active before scheduling jobs
		if (!m_isActive) return;

		// Capture the current job generation to ensure that stale jobs do not run after the scene has been exited or reset
		const uint64_t generation = m_jobGeneration.load(std::memory_order_acquire);

		// Schedule a job to process a chunk of entities for technology diffusion
		JobSystem::Schedule([this, begin, end, dt, generation]() {
			// Check if the scene is active before processing jobs, yes, this is a double-check, but it is necessary to ensure that the scene is still active when the job runs
			if (!m_isActive.load(std::memory_order_acquire) || generation != m_jobGeneration.load(std::memory_order_acquire)) return;

			// Get the list of entities from the entity manager
			auto& ents = m_entityManager.GetEntities();
			
			// Process each entity in the assigned chunk for technology diffusion
			for (size_t j = begin; j < end; ++j) {
				// Check if the scene is still active and the job generation has not changed, which would indicate that the scene has been exited or reset
				if (generation != m_jobGeneration.load(std::memory_order_acquire)) return;

				// Get the entity pointer from the entity manager
				Entity* civ = ents[j].get();

				// Check if the entity is valid and alive before processing it
				if (!civ || !civ->IsAlive()) continue;

				// Get the CCivilisationTech component for the entity
				auto* civTech = civ->GetComponent<CCivilisationTech>();
				
				// Check if the entity has a CCivilisationTech component
				if (!civTech) continue;

				// Process technology diffusion for the civilization using the TechDiffusionSystem
				m_diffusionSystem.ProcessTechDiffusionForCivilisation(civ, civTech, m_entityManager, dt);
			}
		});
	}

	// Advance index for next frame
	m_lastCivIndex = endIndex;
	if (m_lastCivIndex >= entities.size()) m_lastCivIndex = 0; // wrap around
}
/////////////////////////////////