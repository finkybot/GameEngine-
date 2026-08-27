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
	// Civ A
	Entity* civA = m_entityManager.AddEntity(EntityType::Civilisation);
	auto* tA = civA->AddComponent<CTransform>();
	tA->position = Vec2(400, 300);

	auto* techA = civA->AddComponent<CCivilisationTech>();
	techA->knownTechs["agriculture.basic"] = 1.0f;
	techA->unlockedTechs.insert("agriculture.basic");

	// Civ B
	Entity* civB = m_entityManager.AddEntity(EntityType::Civilisation);
	auto* tB = civB->AddComponent<CTransform>();
	tB->position = Vec2(900, 300);

	auto* techB = civB->AddComponent<CCivilisationTech>();
	techB->knownTechs["agriculture.basic"] = 0.0f;
	techB->passiveProgress["agriculture.basic"] = 0.0f;

	// Knowledge Particle
	Entity* particle = m_entityManager.AddEntity(EntityType::KnowledgeParticle);
	auto* kp = particle->AddComponent<CKnowledgeParticle>();
	kp->techId = "agriculture.basic";
	kp->value = 1.0f;

	auto* pT = particle->AddComponent<CTransform>();
	pT->position = Vec2(650, 300);
	pT->velocity = Vec2(0, 0);

	auto* influence = particle->AddComponent<CParticleInfluence>();
	influence->influenceRadius = 500.0f;
	influence->influenceFalloff = 1.0f;
}
/////////////////////////////////



/////////////////////////////////
// Update - Updates the scene state, running the technology systems and rendering the debug window.
void TechSimulationScene::Update(float dt) {
	// Run tech systems
	m_kpSystem.Update(dt, m_entityManager);
	m_diffusionSystem.Update(dt, m_entityManager);
	m_evolutionSystem.Update(dt, m_entityManager);
	m_unlockSystem.Update(dt, m_entityManager);

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

	ImGui::End();
}
/////////////////////////////////

