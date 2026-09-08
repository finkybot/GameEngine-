/////////////////////////////////
#pragma once
#include "Scene.h"
#include "RenderSystemGL.h"
#include "EntityManager.h"
#include "CTransform.h"
#include "CTexture.h"
/////////////////////////////////



/////////////////////////////////
class TestRenderGLScene : public Scene {
public:
	TestRenderGLScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& em);

    void Update(float dt) override;
	void Render() override;
	void DoAction() override;
	void HandleEvent(const std::optional<sf::Event>& event) override;
	void OnEnter() override;
	void OnExit() override;
	void OnWindowResized(sf::Vector2u newSize) override;
	void LoadResources() override;
	void UnloadResources() override;
	void InitialiseGame(sf::Vector2u windowSize) override;

private:
	sf::RenderWindow& m_window;
	RenderSystemGL m_renderGL;
};
/////////////////////////////////