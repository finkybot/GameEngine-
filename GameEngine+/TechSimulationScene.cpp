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
	m_entityManager.ClearAll();  
	InitializeGame(m_gameEngine.windowSize);
}
/////////////////////////////////



/////////////////////////////////
// OnExit - Called when the scene is exited. Currently does nothing, but can be used for cleanup if needed.
void TechSimulationScene::OnExit() {}
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
///////////////////////////////



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
	const int civCount = 100;
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
	const int particleCount = 20;
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
	// --- PARALLEL TECH SYSTEMS ---
	ScheduleTechEvolutionJobs(dt);
	ScheduleTechDiffusionJobs(dt);
	JobSystem::WaitIdle(); // ensure all jobs finish before continuing

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

	auto& entities = m_entityManager.GetEntities();
	for (auto& e : entities) {
		auto* tech = e->GetComponent<CCivilisationTech>();
		if (!tech)
			continue;

		ImGui::Text("Civ %p", e.get());

		const auto knownIt = tech->knownTechs.find("agriculture.basic");
		const float known = (knownIt != tech->knownTechs.end()) ? knownIt->second : 0.0f;

		const auto passiveIt = tech->passiveProgress.find("agriculture.basic");
		const float passive = (passiveIt != tech->passiveProgress.end()) ? passiveIt->second : 0.0f;

		const auto activeIt = tech->activeResearch.find("agriculture.basic");
		const bool isActive = (activeIt != tech->activeResearch.end());
		const float activeProgress = isActive ? activeIt->second : 0.0f;

		const auto rateIt = tech->debugResearchRate.find("agriculture.basic");
		const float researchRate = (rateIt != tech->debugResearchRate.end()) ? rateIt->second : 0.0f;

		const auto boostIt = tech->debugKnowledgeBoost.find("agriculture.basic");
		const float knowledgeBoost = (boostIt != tech->debugKnowledgeBoost.end()) ? boostIt->second : 0.0f;

		const auto diffIt = tech->debugDifficultyFactor.find("agriculture.basic");
		const float difficultyFactor = (diffIt != tech->debugDifficultyFactor.end()) ? diffIt->second : 0.0f;

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
		ImGui::NewLine();

	}

ImGui::SeparatorText("Job System");

	ImGui::Text("Worker Threads: %zu", JobSystem::GetWorkerCount());
	ImGui::Text("Last Frame Job Time: %.3f ms", JobSystem::GetLastFrameJobTimeMs());

	// Rolling graph
	static float history[120] = {0};
	static int index = 0;

	history[index] = (float)JobSystem::GetLastFrameJobTimeMs();
	index = (index + 1) % 120;

	ImGui::PlotLines("Job Time (ms)", history, 120, 0, nullptr, 0.0f, 20.0f, ImVec2(200, 60));

	// --- FPS Display ---
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

	// --- FPS Graph ---
	static float fpsHistory[120] = {0};
	static int fpsIndex = 0;

	fpsHistory[fpsIndex] = ImGui::GetIO().Framerate;
	fpsIndex = (fpsIndex + 1) % 120;

	ImGui::PlotLines("FPS", fpsHistory, 120, 0, nullptr, 0.0f, 200.0f, ImVec2(200, 60));

	ImGui::End();
}
/////////////////////////////////
 
 

/////////////////////////////////
// ScheduleTechEvolutionJobs - Schedules technology evolution jobs based on the elapsed time (dt). Currently does nothing, but can be used for scheduling evolution tasks if needed.
void TechSimulationScene::ScheduleTechEvolutionJobs(float dt) {
	auto& entities = m_entityManager.GetEntities();
	const size_t chunkSize = 64;

	for (size_t i = 0; i < entities.size(); i += chunkSize) {
		size_t begin = i;
		size_t end = std::min(entities.size(), i + chunkSize);

		JobSystem::Schedule([this, begin, end, dt]() {
			auto& ents = m_entityManager.GetEntities();

			for (size_t j = begin; j < end; ++j) {
				Entity* e = ents[j].get();
				if (!e || !e->IsAlive())
					continue;

				auto* tech = e->GetComponent<CCivilisationTech>();
				if (!tech)
					continue;

				m_evolutionSystem.ProcessCivilisationTech(e, tech, m_entityManager, dt);
			}
		});
	}
}
/////////////////////////////////



/////////////////////////////////
// ScheduleTechDiffusionJobs - Schedules technology diffusion jobs based on the elapsed time (dt). Currently does nothing, but can be used for scheduling diffusion tasks if needed.
void TechSimulationScene::ScheduleTechDiffusionJobs(float dt) {
	auto& entities = m_entityManager.GetEntities();
	const size_t chunkSize = 64;

	for (size_t i = 0; i < entities.size(); i += chunkSize) {
		size_t begin = i;
		size_t end = std::min(entities.size(), i + chunkSize);

		JobSystem::Schedule([this, begin, end, dt]() {
			auto& ents = m_entityManager.GetEntities();

			for (size_t j = begin; j < end; ++j) {
				Entity* civ = ents[j].get();
				if (!civ || !civ->IsAlive())
					continue;

				auto* civTech = civ->GetComponent<CCivilisationTech>();
				if (!civTech)
					continue;

				m_diffusionSystem.ProcessTechDiffusionForCivilisation(civ, civTech, m_entityManager, dt);
			}
		});
	}
}
/////////////////////////////////