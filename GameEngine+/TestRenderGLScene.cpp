/////////////////////////////////
#include "TestRenderGLScene.h"
#include "GameEngine.h"
#include "Entity.h"
#include <SFML/Window/Event.hpp>
#include "CTileSprite.h"
/////////////////////////////////


/////////////////////////////////
TestRenderGLScene::TestRenderGLScene(std::string name, sf::RenderWindow& win, World& world)
	: m_window(win), Scene(name, world) {
	// Initialise GL renderer AFTER SFML window exists
	//m_renderGL.Initialise();

	m_screenCamera.position = Vec2(0, 0);
	m_screenCamera.zoom = 1.0f;
	m_screenCamera.viewportWidth = m_window.getSize().x;
	m_screenCamera.viewportHeight = m_window.getSize().y;

	GetWorld().GetEntityManager().GetRenderSystemGL().OnResize(static_cast<int>(world.GetWindowSize().x), static_cast<int>(world.GetWindowSize().y));
}
/////////////////////////////////



/////////////////////////////////
void TestRenderGLScene::OnEnter() {

	// ---------------------------------
	// Step 1: Create test entity quad for rendering
	// ---------------------------------

	// Create first test entity (a simple quad) in the middle of the window
	auto* each = m_world.CreateEntity(EntityType::TeamBoogaloo);

	// Add a CTransform component to the entity and set its position to the center of the window
	auto* trfm = each->AddComponent<CTransform>();
	auto size = GetWorld().GetWindow().getSize();
	trfm->position = Vec2(size.x * 0.5f, size.y * 0.5f);

	// Add a CTexture component to the entity and set its properties for rendering
	auto* tex = each->AddComponent<CTexture>();
	tex->areaW = 120.f;
	tex->areaH = 120.f;
	tex->gpuIndex = 0; // no texture array yet
	tex->color = sf::Color::White;
	tex->visible = true;

	// ---------------------------------
	// Step 2: Create a second test entity (a circle) for rendering
	// ---------------------------------

	// Create a second test entity (a circle) in the top-left corner
	auto* each2 = m_world.CreateEntity(EntityType::TeamBoogaloo);

	// Add a CTransform component to the second entity and set its position to the top-left corner of the window
	auto* trfm2 = each2->AddComponent<CTransform>();
	auto size2 = GetWorld().GetWindow().getSize();
	trfm2->position = Vec2(size2.x * 0.25f, size2.y * 0.25f);

	// Add a CCircleGPU component to the second entity and set its properties for rendering
	auto* circle = each2->AddComponent<CCircleGPU>();
	circle->radius = 50.f;
	circle->color = sf::Color::Green;
	circle->visible = true;

	// ---------------------------------
	// Step 3: Create a third test entity (a text) for rendering
	// ---------------------------------
	// Create a third test entity (a text) in the bottom-right corner (it wont actually be text but a quad for now)
	auto* each3 = m_world.CreateEntity(EntityType::TeamBoogaloo);

	// Add a CTransform component to the third entity and set its position to the bottom-right corner of the window
	auto* trfm3 = each3->AddComponent<CTransform>();
	trfm3->position = Vec2(size.x * 0.02f, size.y * 0.02f);

	// Add a CText component to the third entity and set its properties for rendering
	auto* text = each3->AddComponent<CText>();

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
	GetWorld().GetEntityManager().GetRenderSystemGL().GetTextureManager(texManager);
	texManager->LoadAtlas("terrain", "assets/World_tiles.png", 32, 32);
	texManager->LoadAtlasGL("terrain");

	// ---------------------------------
	// Step 5: Create a tile entity
	// ---------------------------------
	auto* tileEntity = m_world.CreateEntity(EntityType::TeamBoogaloo);

	auto* tileTransform = tileEntity->AddComponent<CTransform>();
	tileTransform->position = Vec2(size.x * 0.5f, size.y * 0.8f);

	auto* tileSprite = tileEntity->AddComponent<CTileSprite>();
	tileSprite->atlasKey = "terrain"; // matches LoadAtlas()
	tileSprite->tileIndex = 0;		  // first tile in atlas
	tileSprite->w = 32.f;
	tileSprite->h = 32.f;
	tileSprite->color = sf::Color::White;
	tileSprite->visible = true;

	// Commit all added entities to EntityManager so they're visible to rendering
	GetWorld().GetEntityManager().ProcessPending();

	//m_renderGL.OnResize(size.x, size.y);
	GetWorld().GetEntityManager().GetRenderSystemGL().OnResize(size.x, size.y);
}
/////////////////////////////////



/////////////////////////////////
void TestRenderGLScene::Update(float dt) {
	// Poll events normally (no ImGui here)
	while (auto eventOpt = GetWorld().GetWindow().pollEvent()) {
		if (eventOpt->is<sf::Event::Closed>())
			GetWorld().GetWindow().close();

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
	//m_renderGL.Render(GetWorld().GetEntityManager());
	GetWorld().GetEntityManager().GetRenderSystemGL().Render(GetWorld().GetEntityManager(), GetActiveCamera());

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

	m_screenCamera.viewportWidth = newSize.x;
	m_screenCamera.viewportHeight = newSize.y;

	// Update GL viewport
	//m_renderGL.OnResize(newSize.x, newSize.y);
	GetWorld().GetEntityManager().GetRenderSystemGL().OnResize(newSize.x, newSize.y);
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
void TestRenderGLScene::Initialise(sf::Vector2u) {}
/////////////////////////////////


