/////////////////////////////////
#pragma once
#include "Scene.h"
#include "RenderSystemGL.h"
#include "EntityManager.h"
#include "CTransform.h"
#include "CTexture.h"
#include <random>
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
	//RenderSystemGL m_renderGL;

	std::mt19937 rng{std::random_device{}()};
	std::uniform_real_distribution<float> randomValue{0.05f, 0.15f};

	float GetRandomFloat() { return randomValue(rng); }
};
/////////////////////////////////