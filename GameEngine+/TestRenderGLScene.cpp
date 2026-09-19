/////////////////////////////////
#include "TestRenderGLScene.h"
#include "GameEngine.h"
#include "Entity.h"
#include <SFML/Window/Event.hpp>
#include "CTileSprite.h"
/////////////////////////////////


/////////////////////////////////
TestRenderGLScene::TestRenderGLScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& em)
	: m_window(win), Scene(engine, em) {
	// Initialise GL renderer AFTER SFML window exists
	//m_renderGL.Initialise();

	m_entityManager.GetRenderSystemGL().OnResize(engine.windowSize.x, engine.windowSize.y);
}
/////////////////////////////////



/////////////////////////////////
void TestRenderGLScene::OnEnter() {

	// ---------------------------------
	// Step 1: Create test entity quad for rendering
	// ---------------------------------

	// Create first test entity (a simple quad) in the middle of the window
	auto* e = m_entityManager.AddEntity(EntityType::TeamBoogaloo);

	// Add a CTransform component to the entity and set its position to the center of the window
	auto* t = e->AddComponent<CTransform>();
	auto size = m_gameEngine.window.getSize();
	t->position = Vec2(size.x * 0.5f, size.y * 0.5f);

	// Add a CTexture component to the entity and set its properties for rendering
	auto* tex = e->AddComponent<CTexture>();
	tex->areaW = 120.f;
	tex->areaH = 120.f;
	tex->gpuIndex = 0; // no texture array yet
	tex->color = sf::Color::White;
	tex->visible = true;

	// ---------------------------------
	// Step 2: Create a second test entity (a circle) for rendering
	// ---------------------------------

	// Create a second test entity (a circle) in the top-left corner
	auto* e2 = m_entityManager.AddEntity(EntityType::TeamBoogaloo);

	// Add a CTransform component to the second entity and set its position to the top-left corner of the window
	auto* t2 = e2->AddComponent<CTransform>();
	auto size2 = m_gameEngine.window.getSize();
	t2->position = Vec2(size2.x * 0.25f, size2.y * 0.25f);

	// Add a CCircleGPU component to the second entity and set its properties for rendering
	auto* circle = e2->AddComponent<CCircleGPU>();
	circle->radius = 50.f;
	circle->color = sf::Color::Green;
	circle->visible = true;

	// ---------------------------------
	// Step 3: Create a third test entity (a text) for rendering
	// ---------------------------------
	// Create a third test entity (a text) in the bottom-right corner (it wont actually be text but a quad for now)
	auto* e3 = m_entityManager.AddEntity(EntityType::TeamBoogaloo);

	// Add a CTransform component to the third entity and set its position to the bottom-right corner of the window
	auto* t3 = e3->AddComponent<CTransform>();
	t3->position = Vec2(size.x * 0.02f, size.y * 0.02f);

	// Add a CText component to the third entity and set its properties for rendering
	auto* text = e3->AddComponent<CText>();

	// Recommended: that we keep multi lines between one to eight lines of text.

	text->text =	"Testing RenderGL Scene - This scene is for testing purposes only.\n"
					"It will use the RenderSystemGL for rendering and display a quad, circle and this text component.\n"
					"Please note that this scene is not intended for production use and may be subject to change or removal in future versions of the engine.\n"
					"RenderGL will eventually replace the lagacy Render Manager on every scene!";



	text->fontKey = "default";

	text->charSize = 18;
	text->color = sf::Color::Red;
	text->visible = true;
	text->align = CText::Align::Left;

    // ---------------------------------
	// Step 4: Load atlas (via RenderSystemGL)
	// ---------------------------------
	TextureManager* texManager = nullptr;
	m_entityManager.GetRenderSystemGL().GetTextureManager(texManager);
	texManager->LoadAtlas("terrain", "assets/World_tiles.png", 16, 16);
	texManager->LoadAtlasGL("terrain");

	// ---------------------------------
	// Step 5: Create a tile entity
	// ---------------------------------
	auto* tileEntity = m_entityManager.AddEntity(EntityType::TeamBoogaloo);

	auto* tileTransform = tileEntity->AddComponent<CTransform>();
	tileTransform->position = Vec2(size.x * 0.5f, size.y * 0.8f);

	auto* tileSprite = tileEntity->AddComponent<CTileSprite>();
	tileSprite->atlasKey = "terrain"; // matches LoadAtlas()
	tileSprite->tileIndex = 0;		  // first tile in atlas
	tileSprite->w = 16.f;
	tileSprite->h = 16.f;
	tileSprite->color = sf::Color::White;
	tileSprite->visible = true;


	//m_renderGL.OnResize(size.x, size.y);
	m_entityManager.GetRenderSystemGL().OnResize(size.x, size.y);
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
	//m_renderGL.Render(m_entityManager);
	m_entityManager.GetRenderSystemGL().Render(m_entityManager);

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
	//m_renderGL.OnResize(newSize.x, newSize.y);
	m_entityManager.GetRenderSystemGL().OnResize(newSize.x, newSize.y);
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