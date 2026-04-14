#include "SpawnSystem.h"
#include "EntityManager.h"
#include "Entity.h"
#include "CExplosion.h"
#include <SFML/System/Clock.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace Spawn;

// Helper function to convert HSV color to RGB. H is in [0,360], S and V are in [0,1]. Output RGB are in [0,255].
static void HSVtoRGB(float h, float s, float v, int& r, int& g, int& b) {
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float rf, gf, bf;
    if (h < 60)       { rf = c; gf = x; bf = 0; }
    else if (h < 120) { rf = x; gf = c; bf = 0; }
    else if (h < 180) { rf = 0; gf = c; bf = x; }
    else if (h < 240) { rf = 0; gf = x; bf = c; }
    else if (h < 300) { rf = x; gf = 0; bf = c; }
    else              { rf = c; gf = 0; bf = x; }
    r = static_cast<int>((rf + m) * 255.0f);
    g = static_cast<int>((gf + m) * 255.0f);
    b = static_cast<int>((bf + m) * 255.0f);
}

SpawnSystem::SpawnSystem(EntityManager* em, sf::RenderWindow* window) : m_entityManager(em), m_window(window) {}
SpawnSystem::~SpawnSystem() {}

// Load a default spawner config for testing/demo purposes
void SpawnSystem::LoadDefault() {
    m_configs.clear();
    SpawnerConfig spawnerConfig;
    spawnerConfig.id = "default";
    spawnerConfig.enabled = true;
    spawnerConfig.pattern = Pattern::LevelScaledCircular;
    spawnerConfig.type = Type::Continuous;
    spawnerConfig.threshold = 0.005f;  // Low threshold to trigger easily
    spawnerConfig.rate = 8.0f;         // 8 spawns per second for continuous
    spawnerConfig.burstCount = 6;
    spawnerConfig.sizeMin = 8.0f;
    spawnerConfig.sizeMax = 32.0f;
    spawnerConfig.spawnRadius = 200.0f;
    spawnerConfig.circularSpeed = 0.15f;
    spawnerConfig.spiralExpansion = 3.0f;
    spawnerConfig.ringCount = 3;
    m_configs.push_back(spawnerConfig);
}

// Load spawner configs from a JSON file. This is a placeholder - actual implementation would parse the file and populate m_configs.
bool SpawnSystem::LoadFromFile(const std::filesystem::path& path, std::string& outError) {
    // Not implemented - return false
    outError = "not implemented"; return false;
}

// Add a new spawner config at runtime
void SpawnSystem::AddConfig(const SpawnerConfig& cfg) { m_configs.push_back(cfg); }

// Update a specific spawner's config by id. Returns true if found and updated, false if not found.
bool SpawnSystem::UpdateConfig(const std::string& id, const SpawnerConfig& cfg) {
    for (auto &config : m_configs) { if (config.id == id) { config = cfg; return true; } }
    return false;
}

// Main update loop for the spawn system. This should be called every frame with the delta time and current level (0.0 to 1.0) to manage spawning logic based on the configured spawners.
void SpawnSystem::Update(float deltaTime, float level) {
	if (!m_enabled) return; // Global enable/disable check

	// Update global time for any time-based patterns/effects
    m_globalTime += deltaTime;

	// Iterate through each spawner config and manage spawning logic based on type and pattern
    for (auto &config : m_configs) {
        if (!config.enabled) continue;
        float &timer = m_spawnTimers[config.id];
        timer += deltaTime;

		// Determine spawning logic based on spawner type
        if (config.type == Type::Burst) {
            // Burst: spawn burstCount entities when level exceeds threshold and cooldown has elapsed
            if (timer >= config.rate && level >= config.threshold) {
                for (int i = 0; i < config.burstCount; ++i) SpawnEntity(config, level);
                timer = 0.0f;
            }
        } else if (config.type == Type::Continuous) { // type == Type::Continuous
            // Continuous: spawn at rate (spawns/sec) while level exceeds threshold
            if (level >= config.threshold) {
                float interval = 1.0f / config.rate;
                while (timer >= interval) { 
                    SpawnEntity(config, level); 
                    timer -= interval; 
                }
            }
        } else if (config.type == Type::Periodic) { // type == Type::Periodic
            // Periodic: spawn at regular intervals regardless of level
            float interval = 1.0f / config.rate;
            while (timer >= interval) { 
                SpawnEntity(config, level); 
                timer -= interval; 
            }
        }
    }
}

// Core function to spawn an entity based on the provided spawner config and current level. 
// Determines spawn position, velocity, size, color, and other properties based on the spawner's pattern and level scaling.
void SpawnSystem::SpawnEntity(const SpawnerConfig& cfg, float level) {
	// Probability check to determine if this spawn should occur based on the configured probability
    if (!m_entityManager) return;

	// Create a new explosion entity. In a full implementation, this could be extended to spawn different types of entities based on the config.
    Entity* entity = m_entityManager->addEntity(EntityType::Explosion);
	if (!entity) return; // Failed to create entity, exit early

	// Base spawn position is the center of the window. This will be modified based on the pattern.
    float centerX = (float)m_window->getSize().x * 0.5f;
    float centerY = (float)m_window->getSize().y * 0.5f;
    float x = centerX, y = centerY, xVelocity = 0.0f, yVelocity = -40.0f;
    float spawnSize = cfg.sizeMin;
    int r=255,g=255,b=255;

    float &angle = m_circularAngles[cfg.id];
    float &srad = m_spiralRadius[cfg.id];
    int &count = m_spawnCounter[cfg.id];
    if (srad <= 0.0f) srad = 20.0f;

    float lvl = level; if (lvl<0) lvl=0; if (lvl>1) lvl=1;

	// Pattern-based spawning logic - can be expanded with more patterns as needed
	// Determine spawn position, velocity, size, and color based on pattern
    switch (cfg.pattern) {
		// Random: original random position/color logic
        case Pattern::Random: {
            spawnSize = cfg.sizeMin + (std::rand()/(float)RAND_MAX)*(cfg.sizeMax-cfg.sizeMin);
            r = 128 + (std::rand()%128); g = 128 + (std::rand()%128); b = 128 + (std::rand()%128);
            float ang = ((float)std::rand()/(float)RAND_MAX)*6.28318f;
            float rad = ((float)std::rand()/(float)RAND_MAX)*cfg.spawnRadius;
            x = centerX + cosf(ang)*rad; y = centerY + sinf(ang)*rad;
            xVelocity = 0.0f; yVelocity = -30.0f - (std::rand()%80);
        } break;

		// Circular: spawn on a circle around the center, with angle advancing each spawn
        case Pattern::Circular: {
            spawnSize = 12.0f + 8.0f * sinf(angle*3.0f);
            r = 128 + (int)(127.0f * sinf(angle+0.0f));
            g = 128 + (int)(127.0f * sinf(angle+2.0f));
            b = 128 + (int)(127.0f * sinf(angle+4.0f));
            x = centerX + cfg.spawnRadius * cosf(angle);
            y = centerY + cfg.spawnRadius * sinf(angle);
            xVelocity = -30.0f * sinf(angle);
            yVelocity = -40.0f - 10.0f * cosf(angle);
            angle += cfg.circularSpeed; if (angle>6.28318f) angle-=6.28318f;
        } break;

		// LevelScaledCircular: similar to Circular but size and brightness scale with level
        case Pattern::LevelScaledCircular: {
            spawnSize = cfg.sizeMin + (cfg.sizeMax-cfg.sizeMin)*lvl;
            float brightness = 0.5f + 0.5f*lvl;
            int ar = 128 + (int)(127.0f * sinf(angle+0.0f));
            int ag = 128 + (int)(127.0f * sinf(angle+2.0f));
            int ab = 128 + (int)(127.0f * sinf(angle+4.0f));
            r = (int)(ar*brightness); if (r>255) r=255;
            g = (int)(ag*brightness); if (g>255) g=255;
            b = (int)(ab*brightness); if (b>255) b=255;
            x = centerX + cfg.spawnRadius * cosf(angle);
            y = centerY + cfg.spawnRadius * sinf(angle);
            xVelocity = -20.0f * sinf(angle) * (0.5f + lvl);
            yVelocity = -30.0f - 40.0f * lvl;
            angle += cfg.circularSpeed; if (angle>6.28318f) angle-=6.28318f;
        } break;

		// Spiral: spawn in an expanding/contracting spiral pattern, with color cycling based on angle
        case Pattern::Spiral: {
            spawnSize = cfg.sizeMin + (cfg.sizeMax-cfg.sizeMin)*(srad/cfg.spawnRadius);
            float hue = fmodf(angle*57.2957795f, 360.0f);
            HSVtoRGB(hue,1.0f,1.0f,r,g,b);
            x = centerX + srad * cosf(angle);
            y = centerY + srad * sinf(angle);
            xVelocity = 20.0f * cosf(angle) + 10.0f * (lvl*2.0f-1.0f);
            yVelocity = 20.0f * sinf(angle) - 20.0f;
            angle += cfg.circularSpeed; srad += cfg.spiralExpansion; if (srad>cfg.spawnRadius) srad=20.0f;
            if (angle>6.28318f) angle-=6.28318f;
        } break;

		// Firework: spawn in a burst outward from the center with random angle, speed, and color, scaled by level
        case Pattern::Firework: {
            spawnSize = cfg.sizeMin + (cfg.sizeMax-cfg.sizeMin)*lvl;
            float hue = 30.0f + (std::rand()%40);
            HSVtoRGB(hue,0.9f,1.0f,r,g,b);
            float startDist = 10.0f + (std::rand()%30);
            float ang = ((float)std::rand()/(float)RAND_MAX)*6.28318f;
            x = centerX + startDist * cosf(ang);
            y = centerY + startDist * sinf(ang);
            float speed = 80.0f + (std::rand()%120) + lvl*100.0f;
            xVelocity = speed * cosf(ang);
            yVelocity = speed * sinf(ang) - 30.0f;
        } break;

		// Figure8: spawn in a Lissajous figure-8 pattern, with size and color cycling based on angle
        case Pattern::Figure8: {
            spawnSize = cfg.sizeMin + (cfg.sizeMax-cfg.sizeMin)*(0.5f + 0.5f * sinf(angle*2.0f));
            float hue = fmodf(angle * 28.6478f, 360.0f);
            HSVtoRGB(hue,0.8f,1.0f,r,g,b);
            x = centerX + cfg.spawnRadius * sinf(angle);
            y = centerY + cfg.spawnRadius * 0.5f * sinf(angle*2.0f);
            xVelocity = 30.0f * cosf(angle);
            yVelocity = 30.0f * cosf(angle*2.0f) - 20.0f;
            angle += cfg.circularSpeed; if (angle>6.28318f) angle-=6.28318f;
        } break;

	    // Wave: spawn in a sine wave pattern across the screen, with horizontal velocity oscillating over time and vertical velocity scaled by level
        case Pattern::Wave: {
            spawnSize = cfg.sizeMin + (cfg.sizeMax-cfg.sizeMin)*lvl;
            float hue = fmodf(count * 15.0f, 360.0f);
            HSVtoRGB(hue,0.7f,1.0f,r,g,b);
            float screenWidth = (float)m_window->getSize().x;
            float xPos = fmodf(count * 30.0f, screenWidth);
            float waveHeight = cfg.spawnRadius * sinf(m_globalTime * 2.0f + xPos * 0.02f);
            x = xPos; y = centerY + waveHeight;
            xVelocity = sinf(m_globalTime) * 20.0f;
            yVelocity = -40.0f - 30.0f * lvl;
            count++;
        } break;

	    // MultiRing: spawn in multiple concentric rings around the center, with size and color based on ring index, and velocity increasing with ring index
        case Pattern::MultiRing: {
            int ringIdx = count % cfg.ringCount;
            float ringRadius = (cfg.spawnRadius / cfg.ringCount) * (ringIdx + 1);
            spawnSize = cfg.sizeMin + (cfg.sizeMax-cfg.sizeMin)*((float)ringIdx / cfg.ringCount);
            float hue = fmodf(ringIdx * 120.0f + angle * 30.0f, 360.0f);
            HSVtoRGB(hue,0.9f,1.0f,r,g,b);
            x = centerX + ringRadius * cosf(angle);
            y = centerY + ringRadius * sinf(angle);
            xVelocity = 15.0f * cosf(angle) * (ringIdx + 1);
            yVelocity = 15.0f * sinf(angle) * (ringIdx + 1) - 25.0f;
            angle += cfg.circularSpeed / (ringIdx + 1);
            if (angle>6.28318f) angle-=6.28318f;
            count++;
        } break;

    	// Starburst: spawn in rays emanating from the center, with position, velocity, size, and color based on ray index and level
        case Pattern::Starburst: {
            int rayCount = 8;
            int rayIdx = count % rayCount;
            float rayAngle = (6.28318f / rayCount) * rayIdx;
            spawnSize = cfg.sizeMin + (cfg.sizeMax-cfg.sizeMin) * (0.5f + 0.5f * sinf(m_globalTime * 3.0f));
            float hue = fmodf(rayIdx * 45.0f, 360.0f);
            HSVtoRGB(hue,1.0f,1.0f,r,g,b);
            float dist = fmodf(count * 5.0f, cfg.spawnRadius);
            x = centerX + dist * cosf(rayAngle);
            y = centerY + dist * sinf(rayAngle);
            float speed = 60.0f + 40.0f * lvl;
            xVelocity = speed * cosf(rayAngle);
            yVelocity = speed * sinf(rayAngle);
            count++;
        } break;

		// Helix: spawn in a double helix pattern with two strands spiraling around each other, with size and color cycling based on angle, and velocity based on strand and level
        case Pattern::Helix: {
            spawnSize = cfg.sizeMin + (cfg.sizeMax-cfg.sizeMin) * (0.5f + 0.5f * cosf(angle * 4.0f));
            int strand = count % 2;
            float strandAngle = angle + (strand * 3.14159f);
            float hue = strand == 0 ? 200.0f : 30.0f;
            hue = fmodf(hue + angle * 10.0f, 360.0f);
            HSVtoRGB(hue,0.9f,1.0f,r,g,b);
            float helixRadius = cfg.spawnRadius * 0.4f;
            x = centerX + helixRadius * cosf(strandAngle);
            float yOffset = fmodf(count * 8.0f, (float)m_window->getSize().y);
            y = (float)m_window->getSize().y - yOffset;
            xVelocity = -40.0f * sinf(strandAngle);
            yVelocity = -50.0f - 30.0f * lvl;
            angle += cfg.circularSpeed; if (angle>6.28318f) angle-=6.28318f;
            count++;
        } break;
    }

	// Create the explosion shape component with the determined size and color, and add it to the entity along with a transform component for position and velocity
    auto circle = std::make_unique<CExplosion>(spawnSize);
    circle->SetColor((float)r,(float)g,(float)b,220);
    entity->AddComponentPtr<CShape>(std::move(circle));
    entity->AddComponent<CTransform>(Vec2(x,y), Vec2(xVelocity,yVelocity));
    m_entityManager->ProcessPending();
}
