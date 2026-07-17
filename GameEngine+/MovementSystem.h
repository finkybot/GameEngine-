cd "F:\Git_Repo\GameEngine+" && .\x64\Release\GameEngine+.exea once#include "Vec2.h"

class Entity;

class MovementSystem {
public:
MovementSystem() = default;
~MovementSystem() = default;

void Update(const std::vector<std::unique_ptr<Entity>>& entities, float deltaTime);

private:
static constexpr float WAYPOINT_ARRIVAL_THRESHOLD = 5.0f;
};
