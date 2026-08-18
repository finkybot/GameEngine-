/////////////////////////////////
// MainMenuScene.cpp - Implementation of the MainMenuScene class, which represents the main menu of the game. This scene allows players to select different scenes to play and manages the lifecycle of the main menu, 
// including handling input, rendering the menu, and transitioning to other scenes based on player selection.
/////////////////////////////////



/////////////////////////////////
// Includes
#include "MainMenuScene.h"
#include "GameEngine.h"
#include <imgui/imgui.h>
#include "FontManager.h"
#include <SFML/Graphics/Text.hpp>

static const std::string kMenuFontName = "default";
/////////////////////////////////



/////////////////////////////////
// Constructor and destructor for the MainMenuScene class. The constructor initializes the scene with references to the game engine, render window, and entity manager, while the destructor handles any necessary cleanup when the scene is destroyed.
MainMenuScene::MainMenuScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager)
	: Scene(engine, entityManager), m_window(win) {}

MainMenuScene::~MainMenuScene() = default;
/////////////////////////////////



/////////////////////////////////
// InitializeGame - Initializes the main menu scene by retrieving the list of available scene names from the game engine, allowing the menu to display options for players to select different scenes to play. This method is called when the scene is 
// first entered and sets up the necessary state for the main menu.
void MainMenuScene::InitializeGame(sf::Vector2u windowSize) {
	m_sceneNames = m_gameEngine.GetSceneNames();
	// Remove the MainMenu entry itself so we don't show an option for the current scene
	m_sceneNames.erase(std::remove(m_sceneNames.begin(), m_sceneNames.end(), std::string("MainMenu")), m_sceneNames.end());
	if (m_selectedIndex >= (int)m_sceneNames.size()) m_selectedIndex = 0;
}
/////////////////////////////////



/////////////////////////////////
// LoadResources and UnloadResources - These methods manage the loading and unloading of resources for the main menu scene. In this implementation, LoadResources simply sets a flag to indicate that resources are loaded, while UnloadResources is empty, 
// as there are no specific resources to manage for the main menu.
void MainMenuScene::LoadResources() { m_isLoaded = true; }
void MainMenuScene::UnloadResources() {}
/////////////////////////////////



/////////////////////////////////
// OnEnter and OnExit - These methods manage the lifecycle of the main menu scene when it is entered and exited. In this implementation, both methods are empty, as there are no specific actions needed when entering or exiting the main menu.
void MainMenuScene::OnEnter() {
	// Disarm Escape-to-close until the key has been physically released after entering
	m_enterCooldown = 0.3f;
	m_escapeArmed = false;
}
void MainMenuScene::OnExit() {}
/////////////////////////////////



/////////////////////////////////
// OnWindowResized - Adjusts the view of the main menu scene when the window is resized, ensuring that the menu remains centered and properly scaled. 
// This method creates a new view based on the new window size and sets it as the current view for rendering.
void MainMenuScene::OnWindowResized(sf::Vector2u newSize) {
	sf::View view;
	view.setCenter(sf::Vector2f(newSize.x * 0.5f, newSize.y * 0.5f));
	view.setSize(sf::Vector2f(newSize.x, newSize.y));
	m_window.setView(view);
}
/////////////////////////////////



/////////////////////////////////
// Update - Updates the main menu scene based on player input for navigating the menu and selecting scenes. This method checks for keyboard input to move the selection up and down the list of available scenes, and if the Enter key is pressed, 
// it changes the current scene to the selected scene using the game engine's ChangeScene method.
void MainMenuScene::Update(float deltaTime) {
	// handle keyboard navigation with simple input debouncing so holding a key doesn't skip multiple entries
	const float repeatDelay = 0.18f; // seconds before first repeat
	const float repeatRate = 0.08f;  // seconds between repeats when held

	static float upTimer = 0.0f;
	static float downTimer = 0.0f;
	static float enterTimer = 0.0f;

	auto handleKey = [&](sf::Keyboard::Key key, float& timer, auto onPress) {
		if (sf::Keyboard::isKeyPressed(key)) {
			timer += deltaTime;
			if (timer >= repeatDelay || (timer > 0.0f && timer >= repeatRate)) {
				onPress();
				timer = 0.0001f; // small non-zero to indicate it's been handled
			}
		} else {
			timer = 0.0f;
		}
	};

	handleKey(sf::Keyboard::Key::Up, upTimer, [&]() {
		m_selectedIndex = std::max(0, m_selectedIndex - 1);
	});
	handleKey(sf::Keyboard::Key::Down, downTimer, [&]() {
		m_selectedIndex = std::min((int)m_sceneNames.size()-1, m_selectedIndex + 1);
	});
	handleKey(sf::Keyboard::Key::Enter, enterTimer, [&]() {
		if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_sceneNames.size()) {
			m_gameEngine.ChangeScene(m_sceneNames[m_selectedIndex]);
		}
	});
	// Escape-to-close: only fires once the key has been fully released after entering
	if (m_enterCooldown > 0.0f) {
		m_enterCooldown -= deltaTime;
	} else {
		if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
			m_escapeArmed = true; // key is up - next press may close
		if (m_escapeArmed && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
			if (m_window.isOpen()) m_window.close();
	}
}
/////////////////////////////////



/////////////////////////////////
// Render - Renders the main menu scene using ImGui to display a list of available scenes for selection. The method creates an ImGui window and displays the scene names as selectable items, allowing players to click on a scene name to change to 
// that scene using the game engine's ChangeScene method.
void MainMenuScene::Render() {
	// Render scene list using the engine FontManager and SFML text so it matches game visuals (not ImGui)
	auto& fm = m_gameEngine.GetFontManager();
	std::shared_ptr<sf::Font> fontPtr;
	if (auto f = fm.GetFont(kMenuFontName)) fontPtr = *f;
	if (!fontPtr) {
		// Fallback: if no engine font is loaded, show the menu via ImGui so the user can still select scenes
		ImGui::Begin("Main Menu");
		ImGui::Text("Select a scene:");
		for (size_t i = 0; i < m_sceneNames.size(); ++i) {
			bool selected = (int)i == m_selectedIndex;
			if (ImGui::Selectable(m_sceneNames[i].c_str(), selected)) {
				m_selectedIndex = (int)i;
				m_gameEngine.ChangeScene(m_sceneNames[i]);
			}
		}
		ImGui::End();
		return;
	}
	float y = 120.0f;
	float x = 80.0f;
	float lineHeight = 32.0f;
	for (size_t i = 0; i < m_sceneNames.size(); ++i) {
		sf::Text txt(*fontPtr);
		txt.setString(m_sceneNames[i]);
		txt.setCharacterSize(24);
		if ((int)i == m_selectedIndex) txt.setFillColor(sf::Color::Yellow);
		else txt.setFillColor(sf::Color::White);
		txt.setPosition(sf::Vector2f(x, y + i * lineHeight));

		m_window.draw(txt);
	}
}
/////////////////////////////////



/////////////////////////////////
// DoAction - This method is a placeholder for any specific actions that the main menu scene might need to perform. In this implementation, it is empty, as there are no specific actions defined for the main menu.
void MainMenuScene::DoAction() {}
/////////////////////////////////



/////////////////////////////////
// HandleEvent - This method is a placeholder for handling any specific events that the main menu scene might need to respond to. In this implementation, it is empty, as the main menu primarily relies on keyboard input for navigation and selection, which is handled in the Update method.
void MainMenuScene::HandleEvent(const std::optional<sf::Event>& event) {
	if (!event.has_value()) return;
	// Handle mouse clicks to select scenes
	if (event->is<sf::Event::MouseButtonPressed>()) {
		if (auto mb = event->getIf<sf::Event::MouseButtonPressed>()) {
			// Only handle left button
			if (static_cast<sf::Mouse::Button>(mb->button) == sf::Mouse::Button::Left) {
				float x = 80.0f;
				float y = 120.0f;
				float lineHeight = 32.0f;
				int mx = mb->position.x;
				int my = mb->position.y;
				if (mx < (int)x) return;
				if (my < (int)y) return;
				int idx = (my - (int)y) / (int)lineHeight;
				if (idx >= 0 && idx < (int)m_sceneNames.size()) {
					m_gameEngine.ChangeScene(m_sceneNames[idx]);
				}
			}
		}
	}
}
/////////////////////////////////
