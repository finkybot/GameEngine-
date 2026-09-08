/////////////////////////////////
#include "TestRenderGLScene.h"
#include "GameEngine.h"
#include "Entity.h"
#include <SFML/Window/Event.hpp>
/////////////////////////////////


/////////////////////////////////
TestRenderGLScene::TestRenderGLScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& em)
	: m_window(win), Scene(engine, em) {
	// Initialise GL renderer AFTER SFML window exists
	m_renderGL.Initialise();
	m_renderGL.OnResize(engine.windowSize.x, engine.windowSize.y);
}
/////////////////////////////////



/////////////////////////////////
void TestRenderGLScene::OnEnter() {
	// Create a single test entity
	auto* e = m_entityManager.AddEntity(EntityType::TeamBoogaloo);

	auto* t = e->AddComponent<CTransform>();
	auto size = m_gameEngine.window.getSize();
	t->position = Vec2(size.x * 0.5f, size.y * 0.5f);

	auto* tex = e->AddComponent<CTexture>();
	tex->areaW = 120.f;
	tex->areaH = 120.f;
	tex->gpuIndex = 0; // no texture array yet
	tex->color = sf::Color::White;
	tex->visible = true;

	m_renderGL.OnResize(size.x, size.y);
}
/////////////////////////////////



/////////////////////////////////
void TestRenderGLScene::Update(float dt) {
	// Poll events normally (no ImGui here)
	while (auto eventOpt = m_gameEngine.window.pollEvent()) {
		if (eventOpt->is<sf::Event::Closed>())
			m_gameEngine.window.close();

		if (eventOpt->is<sf::Event::Resized>()) {
			auto r = eventOpt->getIf<sf::Event::Resized>();
			OnWindowResized(r->size);
		}
	}

	// No physics, no collisions, no spawning — this is a pure GL test
}
/////////////////////////////////



/////////////////////////////////
void TestRenderGLScene::Render() {
	// Draw the GL quad
	m_renderGL.Render(m_entityManager);

	// DO NOT call SFML rendering here
	// GameEngine will still call EntityManager.RenderAll() afterwards,
	// but our test entity has no SFML components, so nothing is drawn twice.
}
/////////////////////////////////



/////////////////////////////////
void TestRenderGLScene::OnWindowResized(sf::Vector2u newSize) {
	// Update SFML view
	sf::View view;
	view.setCenter(sf::Vector2f(newSize.x * 0.5f, newSize.y * 0.5f));
	view.setSize(sf::Vector2f(newSize.x, newSize.y));
	m_window.setView(view);

	// Update GL viewport
	m_renderGL.OnResize(newSize.x, newSize.y);
}
/////////////////////////////////



/////////////////////////////////
void TestRenderGLScene::OnExit() {
	// Nothing to clean up — GL renderer persists
}
/////////////////////////////////



/////////////////////////////////
void TestRenderGLScene::DoAction() {}
void TestRenderGLScene::HandleEvent(const std::optional<sf::Event>&) {}
void TestRenderGLScene::LoadResources() {
	m_isLoaded = true;
}
void TestRenderGLScene::UnloadResources() {}
void TestRenderGLScene::InitialiseGame(sf::Vector2u) {}
/////////////////////////////////