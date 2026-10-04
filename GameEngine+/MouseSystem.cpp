/////////////////////////////////
// MouseSystem.cpp
/////////////////////////////////



/////////////////////////////////
#include "MouseSystem.h"
#include "Entity.h"
#include "Utils.h"
#include "CCamera.h"
#include "CMouseState.h"
/////////////////////////////////



/////////////////////////////////
void MouseSystem::Update(float /*dt*/) {
	auto& entities = m_entityManager->GetEntities();

	// Find main camera
	CCamera* cam = nullptr;
	for (auto& ptr : entities) {
		Entity* e = ptr.get();
		if (!e || !e->IsAlive())
			continue;

		CCamera* c = e->GetComponent<CCamera>();
		if (c && c->isMainCamera) {
			cam = c;
			break;
		}
	}

	if (!cam)
		return; // no camera found

	for (auto& ptr : entities) {
		Entity* e = ptr.get();
		if (!e || !e->IsAlive())
			continue;

		CMouseState* ms = e->GetComponent<CMouseState>();
		if (!ms)
			continue;

		// Screen-space mouse position
		sf::Vector2i mp = sf::Mouse::getPosition(*m_window);
		ms->screenPos = Vec2((float)mp.x, (float)mp.y);

		// Button state
		bool down = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
		ms->leftPressed = (down && !ms->leftDown);
		ms->leftReleased = (!down && ms->leftDown);
		ms->leftDown = down;

		// Convert to world-space using CCamera
		ms->worldPos = ScreenToWorld(ms->screenPos, *cam);
	}
}
/////////////////////////////////