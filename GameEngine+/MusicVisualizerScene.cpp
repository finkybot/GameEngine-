/////////////////////////////////
// MusicVisualizerScene.cpp -	Implementation of the MusicVisualizerScene class, which is responsible for managing the music visualizer 
//								scene in the game. This includes handling music playback, updating visual effects based on the music spectrum, 
//								and managing user input for controlling the visualizer settings.
/////////////////////////////////



/////////////////////////////////
// Includes and namespace aliases for the MusicVisualizerScene implementation. We include necessary headers for SFML graphics and events, as well as the GameEngine, 
// EntityManager, and various component and system classes used in the scene.
#include "MusicVisualizerScene.h"
#include "GameEngine.h"
#include "EntityManager.h"
#include <iostream>
#include "FileDialog.h"
#include "Entity.h"
#include "CTileMap.h"
#include "CTexture.h"
#include "CMusic.h"
#include "CCircle.h"
#include "CExplosion.h"
#include "MusicSystem.h"
#include "Systems/SpawnSystem.h"
#include "CRectangle.h"
#include "CLayer.h"
#include <memory>

// Convert std::filesystem::path to a UTF-8 std::string safely across C++17/20.
// path::u8string() returns std::u8string (char8_t) in C++20, so we reinterpret
// the bytes as char. The underlying UTF-8 encoding is identical.
static std::string PathToUtf8(const std::filesystem::path& p) {
	auto u8 = p.u8string();
	return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}
#include <cmath>
#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>
#include <imgui/imgui_internal.h>
#include <filesystem>
#include <ShlObj.h> // Windows Shell API for known folder paths
/////////////////////////////////



/////////////////////////////////
// SpawnAudioReactiveExplosion - creates a new explosion entity with random size, color, position, and upward velocity. The explosion is spawned 
// around the center of the screen with some random jitter, and its properties are randomized to create a visually interesting effect that reacts to the music. 
// If resetSpawnTimer is true, the spawn timer will be reset to 0 after spawning the explosion, allowing for controlled timing of explosion spawns based on the music spectrum.
void MusicVisualizerScene::SpawnAudioReactiveExplosion(bool resetSpawnTimer) {
	Entity* spawnedEntity = m_entityManager.AddEntity(EntityType::Explosion);
	if (!spawnedEntity)
		return;

	// Random size between 6 and 42
	float size = 6.0f + static_cast<float>(std::rand() % 36);
	auto circle = std::make_unique<CExplosion>(size);

	// Random bright color (RGB each 100-255)
	int r = 100 + (std::rand() % 156);
	int g = 100 + (std::rand() % 156);
	int b = 100 + (std::rand() % 156);
	circle->SetColor(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), 220);
	spawnedEntity->AddComponentPtr<CShape>(std::move(circle));

	// Random position around screen center with jitter
	float cx = static_cast<float>(m_window.getSize().x) * 0.5f;
	float cy = static_cast<float>(m_window.getSize().y) * 0.5f;
	float jitter = 250.0f;
	float x = cx + (static_cast<float>(std::rand() % static_cast<int>(jitter * 2 + 1)) - jitter);
	float y = cy + (static_cast<float>(std::rand() % static_cast<int>(jitter * 2 + 1)) - jitter);

	// Random upward velocity
	float vy = -40.0f - static_cast<float>(std::rand() % 120);
	spawnedEntity->AddComponent<CTransform>(Vec2(x, y), Vec2(0.0f, vy));

	m_entityManager.ProcessPending();
	if (resetSpawnTimer)
		m_spawnTimer = 0.0f;
}
/////////////////////////////////



/////////////////////////////////
// InitializeEqualizerBars - creates or resizes a fixed pool of equalizer bar entities based on the specified visualCount. The bars are evenly spaced 
// across the bottom of the window with some margin on the sides, and they start with a small height and low alpha so they are visible even before the first update.
void MusicVisualizerScene::InitializeEqualizerBars(size_t visualCount) {
	// Remove existing equalizer bars if bandCount changed
	std::vector<Entity*> toRemove;
	for (auto& e : m_entityManager.GetEntities()) {
		if (e->GetType() == EntityType::Equalizer) {
			toRemove.push_back(e.get());
		}
	}
	for (Entity* re : toRemove) {
		m_entityManager.KillEntity(re);
	}
	if (!toRemove.empty()) m_entityManager.Update(0.0f);

	// Create new equalizer bar entities
	float width = static_cast<float>(m_window.getSize().x);
	float margin = 40.0f;
	float usable = width - margin * 2.0f;
    size_t n = (visualCount > 0) ? visualCount : 10;
	float barWidth = usable / static_cast<float>(n);
	float windowHeight = static_cast<float>(m_window.getSize().y);

	// Create 'n' bars evenly spaced across the bottom of the window, with some margin on the sides. Bars start with a small height and low alpha so they 
	// are visible even before the first update.
    for (size_t i = 0; i < n; ++i) {
		Entity* be = m_entityManager.AddEntity(EntityType::Equalizer);
		if (!be) continue;
		float cx = margin + (static_cast<float>(i) + 0.5f) * barWidth;
		float bwidth = barWidth * 0.9f;
        float bheight = 6.0f; // start visible
		float bx = cx - bwidth * 0.5f;
		float by = windowHeight - 20.0f - bheight;
        auto rect = std::unique_ptr<CRectangle>(new CRectangle(bwidth, bheight));
		
		// start with a low alpha so bars are visible even before first update
		rect->SetColor(128.0f + 127.0f * (i / static_cast<float>(n)), 128.0f, 200.0f, 80);
        be->AddComponentPtr<CShape>(std::move(rect));
        
		// Prefer CLayer component for render layer; add it so systems can query layer cheaply
		be->AddComponent<CLayer>(CLayer::Layer::Foreground);
		be->AddComponent<CTransform>(Vec2(bx, by), Vec2::Zero);
		m_entityManager.SetEntityLayer(be, Entity::Layer::Mid);
	}
    m_entityManager.ProcessPending();
    // Resize display values to match pool
	m_eqDisplayValues.clear();
	m_eqDisplayValues.resize(n, 0.0f);
}
/////////////////////////////////



/////////////////////////////////
// HideEqualizerBars - iterates through all equalizer bar entities and sets their size to 1x1 and alpha to 0, effectively hiding them from view. This method 
// is called when the music stops or when the visualizer is toggled off, ensuring that the bars are not visible when they are not active.
void MusicVisualizerScene::HideEqualizerBars() {
	auto& pool = m_entityManager.GetEntities(EntityType::Equalizer);
	for (Entity* e : pool) {
		if (!e) continue;
		if (auto shape = e->GetComponent<CShape>()) {
			// dim and shrink
			if (auto rect = dynamic_cast<CRectangle*>(shape)) {
				rect->SetSize(1.0f, 1.0f);
				rect->SetColor(0.0f, 0.0f, 0.0f, 0);
			}
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// UpdateExplosions - iterates through all explosion entities and updates their size and alpha based on their age. Explosions grow in size and fade out over 
// a lifespan of 1 second, creating a visually appealing effect that reacts to the music.
void MusicVisualizerScene::UpdateEqualizerBars(const std::vector<float>& bands) {
    // allow empty bands to clear/fade bars
	auto& ents = m_entityManager.GetEntities(EntityType::Equalizer);
	size_t n = bands.size();
	size_t m = ents.size();
	if (m == 0) return;
	// Compute layout same as InitializeEqualizerBars
	float width = static_cast<float>(m_window.getSize().x);
	float margin = 40.0f;
	float usable = width - margin * 2.0f;
	float barWidth = usable / static_cast<float>(m);
	float windowHeight = static_cast<float>(m_window.getSize().y);

	// Map spectrum bands to visualizer bars. If there are more bars than bands, some bars will be empty. If there are more bands than bars, 
	// multiple bands will be averaged/interpolated for each bar.
    for (size_t i = 0; i < m; ++i) {
		float level = 0.0f;
        
		// map bar index to spectrum index by scaling to avoid many bars mapping to last band
		// interpolate between nearest bands for smoother distribution
		if (n > 0) {
			float srcPos = (static_cast<float>(i) + 0.5f) * (static_cast<float>(n) / static_cast<float>(m));
			int idx0 = static_cast<int>(std::floor(srcPos));
			int idx1 = idx0 + 1;
			float frac = srcPos - static_cast<float>(idx0);
			if (idx0 < 0) idx0 = 0;
			if (idx1 < 0) idx1 = 0;
			if (idx0 >= static_cast<int>(n)) idx0 = static_cast<int>(n) - 1;
			if (idx1 >= static_cast<int>(n)) idx1 = static_cast<int>(n) - 1;
			float v0 = bands[idx0];
			float v1 = bands[idx1];
			if (!std::isfinite(v0)) v0 = 0.0f;
			if (!std::isfinite(v1)) v1 = 0.0f;
			level = v0 * (1.0f - frac) + v1 * frac;
		}

		// level is expected to be in 0.0 - 1.0 range based on MusicSystem's spectrum processing
		Entity* e = ents[i];
		if (!e) continue;
		auto shape = e->GetComponent<CShape>();
		auto xf = e->GetComponent<CTransform>();
		if (!shape || !xf) continue;
        float bwidth = barWidth * (1.0f - m_eqBarGap);

		// scale bar height to configured fraction of the window for better visibility
        // apply per-bar smoothing (attack/release)
		if (i >= m_eqDisplayValues.size()) m_eqDisplayValues.resize(m, 0.0f);
		float displayed = m_eqDisplayValues[i];

		if (level > displayed) /* attack - rise quickly */ { 
			displayed = displayed * (1.0f - m_eqAttack) + level * m_eqAttack;
		} else /* release - fall slowly */ {
			
			displayed = displayed * (1.0f - m_eqRelease) + level * m_eqRelease;
		}
		m_eqDisplayValues[i] = displayed;

		float bheight = std::max(6.0f, displayed * (windowHeight * m_eqHeightRatio));
		float cx = margin + (static_cast<float>(i) + 0.5f) * barWidth;
		float bx = cx - bwidth * 0.5f;
		float by = windowHeight - 20.0f - bheight;
		
		// Update rectangle size and color/alpha
        // Attempt to resize rectangle shape and update color
		if (auto rectShape = dynamic_cast<CRectangle*>(shape)) {
			rectShape->SetSize(bwidth, bheight);
			int alpha = static_cast<int>(200.0f * std::min(1.0f, level + 0.1f));
			if (alpha < 40) alpha = 40; // ensure minimum visibility
			rectShape->SetColor(128.0f + 127.0f * (i / static_cast<float>(m)), 128.0f + 127.0f * level, 200.0f, alpha);
		}
		xf->position = Vec2(bx, by);
	}
    (void)0; // diagnostics removed
}
/////////////////////////////////



/////////////////////////////////
// SpawnCircularExplosionByLevel - creates a new explosion entity with size, color, and velocity influenced by the provided music level 
// (0.0 - 1.0 expected range). The explosion is spawned in a circular pattern around the center of the screen, 
// with its properties scaled based on the music level to create a dynamic visual effect that reacts to the music spectrum.
void MusicVisualizerScene::SpawnCircularExplosionByLevel(float level, bool resetSpawnTimer) {
	Entity* spawnedEntity = m_entityManager.AddEntity(EntityType::Explosion);
	if (!spawnedEntity)
		return;

	// Clamp level
	if (level < 0.0f)
		level = 0.0f;
	if (level > 1.0f)
		level = 1.0f;

	// Size mapped from small to large based on level
	float minSize = 6.0f;
	float maxSize = 48.0f;
	float size = minSize + (maxSize - minSize) * level;
	auto circle = std::make_unique<CExplosion>(size);

	// Color varies with angle like the regular circular spawner, but scaled by level
	float a = m_circularAngle;
	int ar = 128 + static_cast<int>(127.0f * std::sin(a + 0.0f));
	int ag = 128 + static_cast<int>(127.0f * std::sin(a + 2.0f));
	int ab = 128 + static_cast<int>(127.0f * std::sin(a + 4.0f));
	
	// scale color brightness by level (0..1) where level=0 => 0.5 brightness, level=1 => 1.0 brightness
	float brightness = 0.5f + 0.5f * level;
	auto clamp8 = [](int v) {
		if (v < 0)
			return 0;
		if (v > 255)
			return 255;
		return v;
	};

	// Apply brightness scaling and clamp to valid color range
	int rr = clamp8(static_cast<int>(ar * brightness));
	int gg = clamp8(static_cast<int>(ag * brightness));
	int bb = clamp8(static_cast<int>(ab * brightness));
	circle->SetColor(static_cast<float>(rr), static_cast<float>(gg), static_cast<float>(bb), 220);
	spawnedEntity->AddComponentPtr<CShape>(std::move(circle));

	// Position on circle
	float cx = static_cast<float>(m_window.getSize().x) * 0.5f;
	float cy = static_cast<float>(m_window.getSize().y) * 0.5f;
	float x = cx + m_circularRadius * std::cos(m_circularAngle);
	float y = cy + m_circularRadius * std::sin(m_circularAngle);

	// Velocity slightly influenced by level
	float vx = -20.0f * std::sin(m_circularAngle) * (0.5f + level);
	float vy = -30.0f - 40.0f * level;
	spawnedEntity->AddComponent<CTransform>(Vec2(x, y), Vec2(vx, vy));

	// Advance angle
	m_circularAngle += m_circularSpeed;
	if (m_circularAngle > 3.14159265f * 2.0f)
		m_circularAngle -= 3.14159265f * 2.0f;

	// Commit spawn immediately so it appears in the same frame
	m_entityManager.ProcessPending();
	if (resetSpawnTimer)
		m_spawnTimer = 0.0f;
}
/////////////////////////////////



/////////////////////////////////
// SpawnCircularExplosion - creates a new explosion entity in a deterministic circular pattern around the center of the screen. 
// The explosion's size and color vary based on the current angle in the circular pattern, creating a visually appealing effect 
// that reacts to the music spectrum. Each time this method is called, the angle is advanced by m_circularSpeed, allowing for 
// a continuous circular spawning pattern.
void MusicVisualizerScene::SpawnCircularExplosion(bool resetSpawnTimer) {
	Entity* spawnedEntity = m_entityManager.AddEntity(EntityType::Explosion);
	if (!spawnedEntity)
		return;

	// Fixed size (can vary if desired)
	float size = 12.0f + 8.0f * std::sin(m_circularAngle * 3.0f);
	auto circle = std::make_unique<CExplosion>(size);

	// Color varies with angle for a pleasing effect
	float hue = fmod((m_circularAngle * 180.0f / 3.14159265f), 360.0f);
	int r = 128 + static_cast<int>(127.0f * std::sin(m_circularAngle + 0.0f));
	int g = 128 + static_cast<int>(127.0f * std::sin(m_circularAngle + 2.0f));
	int b = 128 + static_cast<int>(127.0f * std::sin(m_circularAngle + 4.0f));
	circle->SetColor(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), 220);
	spawnedEntity->AddComponentPtr<CShape>(std::move(circle));

	// Compute position on circle around screen center
	float cx = static_cast<float>(m_window.getSize().x) * 0.5f;
	float cy = static_cast<float>(m_window.getSize().y) * 0.5f;
	float x = cx + m_circularRadius * std::cos(m_circularAngle);
	float y = cy + m_circularRadius * std::sin(m_circularAngle);

	// Small outward/inward velocity based on angle to make motion dynamic
	float vx = -30.0f * std::sin(m_circularAngle);
	float vy = -40.0f - 10.0f * std::cos(m_circularAngle);
	spawnedEntity->AddComponent<CTransform>(Vec2(x, y), Vec2(vx, vy));

	// Advance the angle for the next spawn
	m_circularAngle += m_circularSpeed;
	if (m_circularAngle > 3.14159265f * 2.0f)
		m_circularAngle -= 3.14159265f * 2.0f;

	// Commit spawn immediately so it appears in the same frame
	m_entityManager.ProcessPending();
	if (resetSpawnTimer)
		m_spawnTimer = 0.0f;
}
/////////////////////////////////



/////////////////////////////////
// DrawAudioReactiveWindow - renders the ImGui window for controlling the audio reactive spawn settings and visualizer options. This method 
// allows the user to enable or disable the reactive spawn system, toggle the equalizer overlay, and adjust various parameters for how entities 
// are spawned in response to the music spectrum. The window is positioned in the bottom-right corner of the screen and is designed to be an 
// overlay that does not interfere with the main visualizer display.
void MusicVisualizerScene::DrawAudioReactiveWindow() {
	if (!(GImGui && GImGui->WithinFrameScope))
		return;

	// Position the Audio Reactive window in the bottom-right corner with a fixed size (first time only)
	ImVec2 winSize(380, 520);
	ImVec2 pos((float)m_window.getSize().x - winSize.x - 10.0f, (float)m_window.getSize().y - winSize.y - 10.0f);

	// Set next window position and size only on first appearance (ImGuiCond_FirstUseEver), allowing user to move/resize after that
	ImGui::SetNextWindowPos(pos, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(winSize, ImGuiCond_FirstUseEver);
	
	// allow semi-transparent background for this overlay window
	ImGui::SetNextWindowBgAlpha(0.55f);
	ImGui::Begin("Audio Reactive", nullptr, ImGuiWindowFlags_None);
	// make contents scrollable if they overflow
	ImGui::BeginChild("AudioReactiveScroll", ImVec2(0, 0), false, ImGuiWindowFlags_None);

    // Enable/disable spawn system
	bool spawnEnabled = m_spawnSystem ? m_spawnSystem->IsEnabled() : false;
	if (ImGui::Checkbox("Enable Reactive Spawn", &spawnEnabled)) {
		if (m_spawnSystem)
			m_spawnSystem->SetEnabled(spawnEnabled);
	}

	// Single toggle to activate/deactivate the equalizer overlay
	if (ImGui::Checkbox("Equalizer Active", &m_EqualizerActive)) /* Toggle equalizer overlay */ {
		if (m_EqualizerActive) {
			if (m_musicEntity) /* Ensure equalizer pool exists */ {
				if (auto ms = m_entityManager.GetMusicSystem()) {
					auto& pool = m_entityManager.GetEntities(EntityType::Equalizer);
					if (pool.empty()) {
						size_t bands = ms->GetSpectrumBandCount();
						if (bands == 0) bands = 8;
						InitializeEqualizerBars(static_cast<size_t>(m_visualBarCount));
					}
				}
			}
		} else /* release - fall slowly */ {
			HideEqualizerBars();
		}
	}

	// If we have at least one spawner config, allow editing the first one
	if (m_spawnSystem && !m_spawnSystem->GetConfigs().empty()) {
		auto& spawnConfigs = m_spawnSystem->GetConfigsMutable();

		// Spawn type selector
		const char* typeItems[] = {"Burst", "Continuous", "Periodic"};
		int typeIdx = static_cast<int>(spawnConfigs[0].type);
		if (ImGui::Combo("Spawn Type", &typeIdx, typeItems, IM_ARRAYSIZE(typeItems))) {
			Spawn::Type newType = static_cast<Spawn::Type>(typeIdx);
			// Reset rate to appropriate defaults when switching types
			if (newType == Spawn::Type::Burst) {
				// Burst uses cooldown (0.05-2.0s range), set to minimum
				spawnConfigs[0].rate = 0.05f;
			} else {
				// Continuous/Periodic use rate (0.5-30 spawns/s), set to maximum for more visible effect
				spawnConfigs[0].rate = 30.0f;
			}
			spawnConfigs[0].type = newType;
		}

		// Pattern selector (all new patterns)
		const char* patternItems[] = {
			"Random",	 "Circular", "Level-scaled Circular", "Spiral", "Firework", "Figure-8", "Wave", "Multi-Ring",
			"Starburst", "Helix", "Equalizer", "Trippy Tunnel"};
		int patternIdx = static_cast<int>(spawnConfigs[0].pattern);
		if (ImGui::Combo("Spawn Pattern", &patternIdx, patternItems, IM_ARRAYSIZE(patternItems))) {
			spawnConfigs[0].pattern = static_cast<Spawn::Pattern>(patternIdx);
		}

		// Threshold (used by Burst/Continuous)
		if (spawnConfigs[0].type != Spawn::Type::Periodic) {
			ImGui::SliderFloat("Threshold", &spawnConfigs[0].threshold, 0.0001f, 0.5f, "%.4f");
			if (spawnConfigs[0].threshold > 0.1f) {
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Warning: High threshold may prevent spawning!");
			}
		} else {
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1), "Threshold: ignored (Periodic)");
		}

		// Rate / Cooldown
		if (spawnConfigs[0].type == Spawn::Type::Burst) {
			ImGui::SliderFloat("Cooldown (s)", &spawnConfigs[0].rate, 0.05f, 2.0f, "%.2f");
			ImGui::SliderInt("Burst Count", &spawnConfigs[0].burstCount, 1, 40);
		} else {
			ImGui::SliderFloat("Rate (spawns/s)", &spawnConfigs[0].rate, 0.5f, 30.0f, "%.1f");
		}

		// Size controls
		ImGui::SliderFloat("Min Size", &spawnConfigs[0].sizeMin, 2.0f, 50.0f, "%.0f");
		ImGui::SliderFloat("Max Size", &spawnConfigs[0].sizeMax, 10.0f, 100.0f, "%.0f");
		ImGui::SliderFloat("Spawn Radius", &spawnConfigs[0].spawnRadius, 20.0f, 800.0f, "%.0f");

		// Pattern-specific controls
		auto pat = spawnConfigs[0].pattern;
		bool needsRotation = (pat == Spawn::Pattern::Circular || pat == Spawn::Pattern::LevelScaledCircular ||
							  pat == Spawn::Pattern::Spiral || pat == Spawn::Pattern::Figure8 ||
							  pat == Spawn::Pattern::MultiRing || pat == Spawn::Pattern::Helix ||
							  pat == Spawn::Pattern::TripyTunnel);

		// Show rotation speed control if the selected pattern involves rotation. This allows users to adjust how fast the circular/spiral 
		// patterns rotate around the center, which can create different visual effects and better sync with the music.
		if (needsRotation)
			ImGui::SliderFloat("Rotation Speed", &spawnConfigs[0].circularSpeed, 0.01f, 1.0f, "%.2f");

		// Show spiral expansion control if Spiral pattern is selected. This controls how quickly the spiral expands outward with each spawn, 
		// allowing for tighter or looser spirals based on user preference.
		if (pat == Spawn::Pattern::Spiral)
			ImGui::SliderFloat("Spiral Expansion", &spawnConfigs[0].spiralExpansion, 0.1f, 20.0f, "%.1f");
		// Show ring count control if MultiRing or TripyTunnel pattern is selected. For TripyTunnel, this controls symmetry arms (kaleidoscope effect).
		if (pat == Spawn::Pattern::MultiRing || pat == Spawn::Pattern::TripyTunnel) {
			int label = (pat == Spawn::Pattern::TripyTunnel) ? 4 : 1;  // 4-8 for tunnel, 2-8 for rings
			int minVal = (pat == Spawn::Pattern::TripyTunnel) ? 4 : 2;
			const char* labelText = (pat == Spawn::Pattern::TripyTunnel) ? "Symmetry Arms" : "Ring Count";
			ImGui::SliderInt(labelText, &spawnConfigs[0].ringCount, minVal, 8);
		}

		// TripyTunnel-specific movement controls
		if (pat == Spawn::Pattern::TripyTunnel) {
			ImGui::Separator();
			ImGui::Text("Tunnel Movement:");
			float moveSpeed = m_spawnSystem->GetTunnelMoveSpeed();
			if (ImGui::SliderFloat("Move Speed (px/s)", &moveSpeed, 10.0f, 500.0f, "%.0f")) {
				m_spawnSystem->SetTunnelMoveSpeed(moveSpeed);
			}
			float wanderInterval = m_spawnSystem->GetTunnelWanderInterval();
			if (ImGui::SliderFloat("Wander Interval (s)", &wanderInterval, 0.5f, 10.0f, "%.1f")) {
				m_spawnSystem->SetTunnelWanderInterval(wanderInterval);
			}
		}

		ImGui::Separator();
	   ImGui::Text("Spawners: %d", (int)m_spawnSystem->GetConfigs().size());

        // Equalizer visual controls: visual bar count is independent from spectrum band count
		if (auto ms = m_entityManager.GetMusicSystem()) {
			// show current spectrum band count (read-only informational)
			int spectrumBands = static_cast<int>(ms->GetSpectrumBandCount());
			ImGui::Text("Spectrum Bands: %d", spectrumBands);

			// Allow user to configure how many visual bars to display in the equalizer overlay. This controls how many entities are spawned for 
			// the equalizer visualization and how spectrum bands are mapped to them. It can be more or less than the actual spectrum band count 
			// for creative visual effects.
			int visualBars = m_visualBarCount;
			if (ImGui::SliderInt("Visual Bars", &visualBars, 10, 128)) {
				if (visualBars < 1) visualBars = 1;
				if (visualBars > 128) visualBars = 128;
				if (visualBars != m_visualBarCount) {
					m_visualBarCount = visualBars;
					InitializeEqualizerBars(static_cast<size_t>(m_visualBarCount));
				}
			}

			// Show how many equalizer bar entities are currently active
			auto& pool = m_entityManager.GetEntities(EntityType::Equalizer);
			ImGui::Text("Active Equalizer Bars: %d", (int)pool.size());
		}

		// Save/Load preset buttons
		ImGui::Separator();
		ImGui::Text("Presets:");
		if (ImGui::Button("Save Preset")) {
			std::string error;
			if (m_spawnSystem->SaveToFile("assets/spawners/custom.json", error)) {
				m_musicStatus = "Preset saved to assets/spawners/custom.json";
			} else {
				m_musicStatus = "Save failed: " + error;
			}
		}

		// Load buttons for default and custom presets. Default preset is read-only and can be used as a fallback or starting point, while custom 
		// preset allows users to save their own configurations.
		ImGui::SameLine();
		if (ImGui::Button("Load Default")) {
			std::string error;
			if (m_spawnSystem->LoadFromFile("assets/spawners/default.json", error)) {
				m_musicStatus = "Loaded default preset";
			} else {
				m_musicStatus = "Load failed: " + error;
			}
		}

		// Custom preset load allows users to persist their own configurations across sessions. It will overwrite the current spawn system configuration 
		// with the one loaded from the file, so users can experiment and then save if they like the changes.
		ImGui::SameLine();
		if (ImGui::Button("Load Custom")) {
			std::string error;
			if (m_spawnSystem->LoadFromFile("assets/spawners/custom.json", error)) {
				m_musicStatus = "Loaded custom preset";
			} else {
				m_musicStatus = "Load failed: " + error;
			}
		}

        // Spectrum / EQ settings (if music system present)
		if (auto ms = m_entityManager.GetMusicSystem()) {
			// FFT enable/disable
			bool useFFT = ms->GetUseFFT();
			if (ImGui::Checkbox("Use FFT Analysis", &useFFT)) {
				ms->SetUseFFT(useFFT);
			}

			// FFT size (power of two enforced by setter)
			int fftSize = ms->GetFFTSize();
			if (ImGui::InputInt("FFT Size", &fftSize)) {
				if (fftSize < 16) fftSize = 16;
				if (fftSize > 16384) fftSize = 16384;
				ms->SetFFTSize(fftSize);
			}

			// Spectrum band count (visual mapping)
			int bands = static_cast<int>(ms->GetSpectrumBandCount());
			if (ImGui::SliderInt("Spectrum Bands", &bands, 8, 128)) {
				ms->SetSpectrumBandCount(bands);
			}

			// Smoothing
			float smooth = ms->GetSpectrumSmoothing();
			if (ImGui::SliderFloat("Spectrum Smoothing", &smooth, 0.0f, 0.95f, "%.2f")) {
				ms->SetSpectrumSmoothing(smooth);
			}

			ImGui::Text("FFT: %s  Size: %d", ms->GetUseFFT() ? "On" : "Off", ms->GetFFTSize());
		}

		// Quick actions for Equalizer
		if (ImGui::Button("Activate Equalizer")) {
			spawnConfigs[0].pattern = Spawn::Pattern::Equalizer;
			spawnConfigs[0].type = Spawn::Type::Continuous;
			spawnConfigs[0].rate = 12.0f; // reasonable default
			spawnConfigs[0].spawnRadius = 300.0f;
          // create visual pool using configured visual bar count and activate equalizer
			InitializeEqualizerBars(static_cast<size_t>(m_visualBarCount));
			m_musicStatus = "Equalizer activated on spawner 0";
			m_EqualizerActive = true;
		}

		// Deactivate equalizer by switching back to a default random pattern and hiding bars. This allows users to easily toggle the visualizer on and off without 
		// losing their spawn configuration settings, as they can switch back to the Equalizer pattern to reactivate it with the same parameters.
		ImGui::SameLine();
		if (ImGui::Button("Disable Equalizer")) {
			spawnConfigs[0].pattern = Spawn::Pattern::Random;
			m_musicStatus = "Equalizer disabled";
            m_EqualizerActive = false;
			HideEqualizerBars();
		}
	} else {
		ImGui::TextUnformatted("No spawners configured.");
	}

	// Audio-Reactive Effect Controls
	ImGui::Separator();
	ImGui::Text("Audio-Reactive Effects");
	ImGui::SliderFloat("Reactivity Sens##audio", &m_reactivitySensitivity, 0.1f, 5.0f, "%.2f");
	ImGui::SliderFloat("Velocity Scale##audio", &m_velocityScale, 0.1f, 3.0f, "%.2f");
	ImGui::SliderFloat("Burst Intensity##audio", &m_burstIntensity, 0.0f, 3.0f, "%.2f");
	ImGui::SliderFloat("Beat Threshold##audio", &m_beatThreshold, 0.05f, 1.0f, "%.2f");

	// Debug: Display current reactivity values in SpawnSystem
	if (m_spawnSystem) {
		ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "SpawnSystem Reactivity:");
		ImGui::Text("  Sens: %.2f | Vel: %.2f | Burst: %.2f | Beat: %.2f", 
			m_spawnSystem->GetReactivitySensitivity(),
			m_spawnSystem->GetVelocityScale(),
			m_spawnSystem->GetBurstIntensity(),
			m_spawnSystem->GetBeatThreshold());
	}

	// Show current music level for reference
	float level = 0.0f;
	bool hasBuffer = false;
	if (m_musicEntity) {
		if (auto ms = m_entityManager.GetMusicSystem()) {
			// Avoid calling Process() from UI code; Update() already processes once per frame.
			level = ms->GetLevel(m_musicEntity->GetId());
			hasBuffer = ms->HasAnalysisBuffer(m_musicEntity->GetId());
		}
	}
	ImGui::Text("Level: %.4f", level);
	ImGui::SameLine();
	ImGui::TextUnformatted(hasBuffer ? "(analyzing)" : "(no analysis)");

	// Debug: Show calculated rate multiplier
	if (m_spawnSystem) {
		float clampedLevel = std::clamp(level, 0.0f, 1.0f);
		float rateMultiplier = 0.3f + clampedLevel * (1.0f + m_reactivitySensitivity);
		float velocityMultiplier = 0.5f + clampedLevel * m_velocityScale;
		ImGui::TextColored(ImVec4(0.2f, 1.0f, 1.0f, 1.0f), "Multipliers: Rate=%.2fx | Vel=%.2fx", 
			rateMultiplier, velocityMultiplier);

		// Check if level is below threshold
		auto& configs = m_spawnSystem->GetConfigs();
		if (!configs.empty()) {
			if (level < configs[0].threshold) {
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), 
					"Note: Level (%.4f) below threshold (%.4f) - no spawning!", level, configs[0].threshold);
			}
		}
	}

	// end scrollable child and window
	ImGui::EndChild();
	ImGui::End();
}
/////////////////////////////////



/////////////////////////////////
// DrawPlaybackControls - renders the ImGui controls for music playback, including play/pause/stop buttons, volume slider, loop toggle, and playhead 
// slider for seeking. The controls reflect the current state of the music entity and allow the user to interactively control playback.
void MusicVisualizerScene::DrawPlaybackControls() {
	// Display music status and controls
	if (!m_musicStatus.empty())
		ImGui::TextUnformatted(m_musicStatus.c_str());

	// If a music entity exists
	if (m_musicEntity) {
		// Get the CMusic component from the music entity
		if (auto musicCmp = m_musicEntity->GetComponent<CMusic>()) {
			// Display the current state of the music
			const char* stateText = "Unknown"; // unknown state
			switch (musicCmp->state) {
			case CMusic::State::Playing:
				stateText = "Playing";
				break;
			case CMusic::State::Paused:
				stateText = "Paused";
				break;
			case CMusic::State::Stopped:
				stateText = "Stopped";
				break;
			}
			// set the text to show the current state of the music
			ImGui::Text("State: %s", stateText);

			// Playback control buttons: change the music state and process immediately
			if (ImGui::Button("Play")) {
				musicCmp->state = CMusic::State::Playing;
				std::cout << "[MusicVisualizer] Play pressed for entity "  << (m_musicEntity ? m_musicEntity->GetId() : 0) << std::endl;

				// If the track has ended and looping is disabled, request a restart next update
				if (auto musicSys = m_entityManager.GetMusicSystem()) {
					float pos = musicSys->GetPlayingOffset(m_musicEntity->GetId());
					float dur = musicSys->GetDuration(m_musicEntity->GetId());
					if (dur > 0.0f && pos >= dur - 0.05f) {
						m_requestRestart = true;
					}
					musicSys->Process();
				}
			} // Play music
			
			ImGui::SameLine();
			if (ImGui::Button("Pause")) {
				musicCmp->state = CMusic::State::Paused;
				if (auto musicSys = m_entityManager.GetMusicSystem())
					musicSys->Process();
			} // Pause music
			
			ImGui::SameLine();
			if (ImGui::Button("Stop")) {
				musicCmp->state = CMusic::State::Stopped;
				if (auto musicSys = m_entityManager.GetMusicSystem())
					musicSys->Process();
			} // Stop music

			// Volume slider: need to process any change in order for it to apply immediately
			float vol = musicCmp->volume;
			if (ImGui::SliderFloat("Volume", &vol, 0.0f, 100.0f, "%.0f")) {
				musicCmp->volume = vol;
				if (auto musicSys = m_entityManager.GetMusicSystem())
					musicSys->Process();
			} // Adjust volume

			// 3D Audio positioning controls
			ImGui::Separator();
			ImGui::Text("3D Audio Position (Experimental)");

			if (ImGui::SliderFloat("Music X Position", &m_musicX, 0.0f, static_cast<float>(m_window.getSize().x), "%.0f")) {
				if (auto transform = m_musicEntity->GetComponent<CTransform>()) {
					transform->position.x = m_musicX;
				}
			}

			if (ImGui::SliderFloat("Music Y Position", &m_musicY, 0.0f, static_cast<float>(m_window.getSize().y), "%.0f")) {
				if (auto transform = m_musicEntity->GetComponent<CTransform>()) {
					transform->position.y = m_musicY;
				}
			}

			if (ImGui::SliderFloat("Min Distance##music", &m_musicMinDistance, 100.0f, 2000.0f, "%.0f")) {
				if (auto musicCmp = m_musicEntity->GetComponent<CMusic>()) {
					musicCmp->m_3DMinDistance = m_musicMinDistance;
				}
			}

			if (ImGui::SliderFloat("Max Distance##music", &m_musicMaxDistance, 500.0f, 10000.0f, "%.0f")) {
				if (auto musicCmp = m_musicEntity->GetComponent<CMusic>()) {
					musicCmp->m_3DMaxDistance = m_musicMaxDistance;
				}
			}
		}
	}

	// Loop toggle and playhead / seek
	if (m_musicEntity) {
		if (auto musicCmp = m_musicEntity->GetComponent<CMusic>()) {
			// Access music through MusicSystem to avoid race conditions
			if (auto musicSys = m_entityManager.GetMusicSystem()) {
				m_playhead = musicSys->GetPlayingOffset(m_musicEntity->GetId());
				m_duration = musicSys->GetDuration(m_musicEntity->GetId());
			}
			m_loopEnabled = musicCmp->loop;
		}

		// Loop checkbox - process immediately when changed
		if (ImGui::Checkbox("Loop", &m_loopEnabled)) {
			if (auto musicCmp = m_musicEntity->GetComponent<CMusic>()) {
				musicCmp->loop = m_loopEnabled;
				// Process immediately to update sf::Music looping state
				if (auto musicSys = m_entityManager.GetMusicSystem())
					musicSys->Process();
			}
		}

		// Playhead slider (seek)
		if (m_duration > 0.0f) {
			float newPos = m_playhead;
			ImGui::SliderFloat("Playhead (s)", &newPos, 0.0f, m_duration, "%.2f");

			// Detect drag start/end using IsItemActive
			bool sliderActive = ImGui::IsItemActive();

			// On drag start, if music is playing, pause it and remember to resume on drag end.
			if (sliderActive && !m_playheadActive) {
				if (auto cm = m_musicEntity->GetComponent<CMusic>()) {
					m_wasPlayingBeforeSeek = (cm->state == CMusic::State::Playing);
					if (m_wasPlayingBeforeSeek) {
						cm->state = CMusic::State::Paused;
						if (auto musicSys = m_entityManager.GetMusicSystem())
							musicSys->Process();
					}
				}
			}

			// While dragging, only update UI state and defer actual seek until release.
			if (sliderActive && newPos != m_playhead) {
				m_pendingSeekPos = newPos;
				m_seekPending = true;
				m_playhead = newPos;
			}

			// On drag end, apply one seek and then resume playback if needed.
			if (!sliderActive && m_playheadActive) {
				if (m_seekPending) {
					if (auto musicSys = m_entityManager.GetMusicSystem()) {
						musicSys->Seek(m_musicEntity->GetId(), m_pendingSeekPos);
					}
					m_seekPending = false;
				}

				if (m_wasPlayingBeforeSeek) {
					if (auto musCmp = m_musicEntity->GetComponent<CMusic>()) {
						musCmp->state = CMusic::State::Playing;
						if (auto musicSys = m_entityManager.GetMusicSystem())
							musicSys->Process();
					}
				}
			}

			// Set the playhead active state, true if we are currently dragging the playhead, and false after release.
			m_playheadActive = sliderActive;

			// Display the current playhead position and duration
			ImGui::SameLine();
			ImGui::Text("/ %.2fs", m_duration);
		} else {
			ImGui::TextUnformatted("Playhead: n/a");
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// ShowOpenFileBrowser - handles the ImGui UI for browsing and selecting audio files to load into the music visualizer. When the user clicks the "Browse..." button, 
// a modal popup appears with a file browser interface that allows navigation through directories, filtering of audio files, and selection of a music file. Upon 
// selecting a valid audio file, it is loaded into the scene and assigned to a music entity with a CMusic component for playback and analysis.
void MusicVisualizerScene::ShowOpenFileBrowser() {
	ImGui::SameLine(); // Keep the "Browse..." button on the same line as the previous UI elements

	// Try and create a button to open the file browser popup
	if (ImGui::Button("Browse...")) {
		m_showOpenDialog = true;
		ImGui::OpenPopup("Open Audio File");
	}

	// If the popup is open then display the file browser UI
	if (m_showOpenDialog && ImGui::BeginPopupModal("Open Audio File", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		// Static variables to hold the state of the file browser across frames
		static std::vector<std::filesystem::directory_entry> entries;
		static std::string selectedPath;
		static std::string refreshError;
		static int skippedCount = 0;
		static bool showNonAudio = false;
		static std::filesystem::path lastRefreshedDir; // Track which directory we last refreshed

		// If we haven't refreshed the current directory yet, or if the current directory has changed since the last refresh, normalize to an absolute path before comparing. 
		// This avoids subtle mismatches due to relative vs absolute or trailing slash differences that can prevent a refresh when the user navigates into a directory.
		std::filesystem::path normCur;
		try {
			normCur = std::filesystem::absolute(m_currentDir);
		} catch (...) {
			normCur = m_currentDir;
		}

		// Refresh the directory listing if we haven't done so for the current directory yet, or if the current directory has changed since the last refresh
		if (entries.empty() || lastRefreshedDir != normCur) {
			if (!RefreshDirectoryListing(normCur, entries, refreshError, skippedCount, showNonAudio)) {
				// on failure entries will be empty and refreshError contains the message
			}
			lastRefreshedDir = normCur;
			// ensure m_currentDir stores the normalized value too
			m_currentDir = normCur;
		}

		// Drive selection combo box: On Windows (I'm not planning this for other platforms but....), Users can select different drives (C:\, D:\, etc.) so populate 
		// a list of available drives and show it in a combo box. When the user selects a drive, change the current directory to the given drive and refresh the entries.
		std::vector<std::string> drives;

		// Check for drives and add them to the list if they exist. Using std::filesystem::exists to check if the root of the drive exists
		for (char d = 'A'; d <= 'Z'; ++d) {
			std::string root;
			root.push_back(d);
			root += ":\\";
			std::error_code errorCode;
			if (std::filesystem::exists(root, errorCode))
				drives.push_back(root);
		}

		// If we have any drives, show the combo box for drive selection and refresh the directory listing.
		static int selDrive = -1;
		if (!drives.empty()) {
			if (selDrive < 0 || selDrive >= (int)drives.size())
				selDrive = 0;
			std::string items;
			for (size_t i = 0; i < drives.size(); ++i) {
				items += drives[i];
				items.push_back('\0');
			}
			if (ImGui::Combo("Drive", &selDrive, items.c_str())) {
				m_currentDir = drives[selDrive];
				RefreshDirectoryListing(m_currentDir, entries, refreshError, skippedCount, showNonAudio);
				lastRefreshedDir = m_currentDir;
			}
		}

		// Checkbox to toggle showing non-audio files. When toggled, it will refresh the directory listing to apply the new filter.
		ImGui::SameLine();
		ImGui::Checkbox("Show non-audio files", &showNonAudio);

		// Search box to filter filenames shown in the file browser
		static char searchBuf[256] = "";
		ImGui::SameLine();
		ImGui::PushItemWidth(200);
		if (ImGui::InputText("Search", searchBuf, sizeof(searchBuf))) {
			/* user typed, we'll filter display */
		}

		// Clear search button to reset the search filter and show all files again
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button("Clear")) {
			searchBuf[0] = '\0';
		}

		// Display the current folder path. Provide an "Up" button to navigate to the parent directory, which updates the current directory and refreshes the listing. 
		// Also provide a "Refresh" button to manually refresh the directory listing in case of external changes.
		std::string currentDirStr;
		try {
			currentDirStr = m_currentDir.empty() ? "(empty)" : PathToUtf8(m_currentDir);
		} catch (...) {
			currentDirStr = "(invalid)";
		}
		ImGui::Text("Current folder: %s", currentDirStr.c_str());
		if (ImGui::Button("Up") && m_currentDir.has_parent_path()) {
			m_currentDir = m_currentDir.parent_path();
			RefreshDirectoryListing(m_currentDir, entries, refreshError, skippedCount, showNonAudio);
			lastRefreshedDir = m_currentDir;
		}
		// Refresh button to manually refresh the directory listing.
		ImGui::SameLine();
		if (ImGui::Button("Refresh")) {
			RefreshDirectoryListing(m_currentDir, entries, refreshError, skippedCount, showNonAudio);
			lastRefreshedDir = m_currentDir;
		}

		// Display any errors that occur during directory reading or refreshing, as well as the count of skipped entries due to permissions or other issues. Finally, 
		// show the count of items being displayed.
		if (!refreshError.empty()) {
			ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "%s", refreshError.c_str());
		}
		if (skippedCount > 0) {
			ImGui::TextColored(ImVec4(1, 0.7f, 0, 1), "Skipped %d entries (permissions/special chars)", skippedCount);
		}
		// We'll show the filtered count below after rendering the list

		// Child region to display the list of files and directories. Each entry is selectable, and double-clicking a directory will navigate into it, while double-clicking 
		// a file will select it for loading.
		ImGui::BeginChild("file_list", ImVec2(600, 300), true);
		// Track how many entries we actually display after applying the search filter
		int displayedCount = 0;
		std::string queryLower;

		// Convert search query to lowercase for case-insensitive comparison. Using try/catch to handle any issues with the search string
		try {
			queryLower = std::string(searchBuf);
		} catch (...) {
			queryLower.clear();
		}

		// Iterate through the directory entries and display them in the list. Apply the search filter to only show entries that match the query. Handle any exceptions that may occur
		for (auto& ent : entries) {
			// Apply search filter (case-insensitive substring). If empty, show all.
			// Validate the entry path before attempting to call filename() on it
			std::string nameTry;
			bool entryValid = false;
			try {
				// Create a defensive copy of the path to avoid use-after-free from stale directory_entry
				std::filesystem::path entPathCopy;
				try {
					entPathCopy = ent.path();
					entryValid = true;
				} catch (...) {
					// If path() itself fails, skip this entry
					entryValid = false;
				}

				// Check if path is empty before accessing it to prevent invalid pointer dereference
				if (entryValid && !entPathCopy.empty()) {
					nameTry = PathToUtf8(entPathCopy.filename());
				} else {
					nameTry.clear();
				}
			} catch (...) {
				nameTry.clear();
			}

			if (!entryValid) {
				continue; // Skip invalid entries
			}

			std::string nameLower = nameTry;
			for (auto& c : nameLower)
				c = (char)tolower((unsigned char)c);

			std::string qLower = queryLower;
			for (auto& c : qLower)
				c = (char)tolower((unsigned char)c);

			if (!qLower.empty() && nameLower.find(qLower) == std::string::npos)
				continue;
			std::string name;
			try {
				// Create a defensive copy of the path to avoid use-after-free from stale directory_entry
				std::filesystem::path entPathCopy;
				try {
					entPathCopy = ent.path();
					if (entPathCopy.empty()) {
						name = "<invalid>";
					} else {
						name = PathToUtf8(entPathCopy.filename());
					}
				} catch (...) {
					name = "<unreadable>";
				}
			} catch (...) {
				name = "<unreadable>";
			}

			bool is_dir = false;
			std::error_code ec;
			try {
				is_dir = ent.is_directory(ec);
			} catch (...) {
				is_dir = false;
			}
			std::string label = is_dir ? (name + "/") : name;
			std::string entPathStr;
			try {
				// Create a defensive copy of the path to avoid use-after-free from stale directory_entry
				std::filesystem::path entPathCopy;
				try {
					entPathCopy = ent.path();
					if (entPathCopy.empty()) {
						entPathStr.clear();
					} else {
						entPathStr = PathToUtf8(entPathCopy);
					}
				} catch (...) {
					entPathStr.clear();
				}
			} catch (...) {
				entPathStr.clear();
			}
			bool selected = (!selectedPath.empty() && selectedPath == entPathStr);

			// Render the selectable entry. If it's selected, update the selectedPath. If it's a directory and we double-click it, navigate into it and refresh the listing. 
			// If it's a file and we double-click it, select it for loading.
			if (ImGui::Selectable(label.c_str(), selected)) {
				if (!entPathStr.empty())
					selectedPath = entPathStr;
				if (is_dir) {
					// normalize directory we are entering to absolute form to avoid later mismatches
					try {
						// Create a defensive copy of the path
						std::filesystem::path entPathCopy;
						try {
							entPathCopy = ent.path();
							m_currentDir = std::filesystem::absolute(entPathCopy);
						} catch (...) {
							try {
								m_currentDir = ent.path();
							} catch (...) {
								// If all else fails, don't navigate
							}
						}
					} catch (...) {
						// Navigation failed, continue without changing directory
					}
					refreshError.clear();
					skippedCount = 0;
					RefreshDirectoryListing(m_currentDir, entries, refreshError, skippedCount, showNonAudio);
					lastRefreshedDir = m_currentDir;
					selectedPath.clear();
				}
			}

			++displayedCount;

			// Handle double-click to open file. Use IsItemHovered() together with IsMouseDoubleClicked
			if (!is_dir && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				std::string sel = selectedPath;

				// Load a file immediately on double-click: if it's a valid selection; create an entity and add a CMusic component with the selected file, 
				// finally close the popup and reset browser state.
				if (!sel.empty()) {
					// kill and music entity that might exist, don't want multiple music entities
					if (m_musicEntity) {
						m_entityManager.KillEntity(m_musicEntity);
						m_entityManager.Update(0.0f);
						m_musicEntity = nullptr;
					}

					// create a new music entity with a CMusic component for the selected file
					Entity* musicEntity = m_entityManager.AddEntity(EntityType::Default);
					// add the componenent and music if we have an entity
					if (musicEntity) {
						LoadMusicFromPath(sel);
					}
				}
				ImGui::CloseCurrentPopup();
				m_showOpenDialog = false;
				break;
			}
		}
		ImGui::EndChild();
		ImGui::Separator();
		ImGui::Text("Showing %d items (filtered: %d)", (int)entries.size(), displayedCount);

		// OK and Cancel buttons: OK will load the selected file, Cancel will just close the popup.
		if (ImGui::Button("OK") && !selectedPath.empty()) {
			std::string sel = selectedPath;
			if (m_musicEntity) {
				m_entityManager.KillEntity(m_musicEntity);
				m_entityManager.Update(0.0f);
				m_musicEntity = nullptr;
			}
			Entity* musicEntity = m_entityManager.AddEntity(EntityType::Default);

			// add the componenent and music if we have an entity bla bla bla, same as the double-click handler (might be better to refactor this into a helper function)
			if (musicEntity) {
				auto* musicComponent = musicEntity->AddComponent<CMusic>(sel, 80.f, true, true);
				musicComponent->state = CMusic::State::Playing;
				musicComponent->loop = true;
				m_musicEntity = musicEntity;
				m_entityManager.ProcessPending();
				if (auto ms = m_entityManager.GetMusicSystem())
					ms->Process();
				m_musicStatus = std::string("Loaded: ") + sel;
				std::cout << m_musicStatus << std::endl;
			}
			ImGui::CloseCurrentPopup();
			m_showOpenDialog = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
			m_showOpenDialog = false;
		}
		ImGui::EndPopup();
	}
}
/////////////////////////////////



/////////////////////////////////
// RefreshDirectoryListing - helper function to read the contents of a directory and populate a list of entries for the file browser. It takes the directory path, an output vector 
// for entries, an output string for error messages, an output int for skipped entry count, and a flag to show non-audio files. It performs validation on the directory path, iterates 
// through the directory while handling errors gracefully, filters entries based on audio file extensions (unless showNonAudio is true), and sorts the entries with directories first 
// followed by files in alphabetical order. It returns true on success or false on failure with an appropriate error message.
bool MusicVisualizerScene::RefreshDirectoryListing(const std::filesystem::path& dir,
												   std::vector<std::filesystem::directory_entry>& outEntries,
												   std::string& outError, int& outSkipped, bool showNonAudio) {
	outEntries.clear();
	outError.clear();
	outSkipped = 0;

	// Basic validation
	try {
		auto native = dir.native();
		if (native.empty()) {
			outError = "Directory path empty";
			return false;
		}
		if (native.size() > 32768) {
			outError = "Directory path too long";
			return false;
		}
	} catch (...) {
		outError = "Invalid directory path";
		return false;
	}

	// Set options to skip entries we don't have permission to access and to follow directory symlinks. This allows us to avoid exceptions for permission 
	// issues and still show the contents of symlinked directories.
	std::filesystem::directory_options opts = std::filesystem::directory_options::skip_permission_denied |
											  std::filesystem::directory_options::follow_directory_symlink;

	// Iterate through the directory entries with error handling. For each entry, we check if it's a directory or an audio file (based on extension) and add it to the list.
	// If we encounter errors while accessing an entry (e.g., permissions), we increment the skipped count and continue without adding it to the list.
	try {
		std::error_code dirEc;
		std::filesystem::directory_iterator it(dir, opts, dirEc);
		if (dirEc) {
			outError = std::string("Error opening directory: ") + dirEc.message();
			return false;
		}

		for (auto& entry : it) {
			std::error_code ec_entry;
			bool is_dir = false;
			try {
				is_dir = entry.is_directory(ec_entry);
			} catch (...) {
				++outSkipped;
				continue;
			}
			if (ec_entry) {
				++outSkipped;
				continue;
			}
			if (is_dir) {
				outEntries.push_back(entry);
				continue;
			}
			if (showNonAudio) {
				outEntries.push_back(entry);
				continue;
			}
			std::string fileExt;
			try {
				fileExt = entry.path().extension().string();
			} catch (...) {
				fileExt.clear();
			}
			for (auto& c : fileExt)
				c = (char)tolower((unsigned char)c);
			if (fileExt == ".mp3" || fileExt == ".ogg" || fileExt == ".wav" || fileExt == ".flac")
				outEntries.push_back(entry);
		}
	} catch (const std::exception& ex) {
		outError = std::string("Error iterating directory: ") + ex.what();
		outEntries.clear();
		return false;
	} catch (...) {
		outError = "Unknown error iterating directory";
		outEntries.clear();
		return false;
	}

	try {
		std::sort(outEntries.begin(), outEntries.end(), [](auto const& a, auto const& b) {
			std::error_code ea, eb;
			bool ad = false, bd = false;
			try {
				ad = std::filesystem::is_directory(a.path(), ea);
			} catch (...) {
				ad = false;
			}
			try {
				bd = std::filesystem::is_directory(b.path(), eb);
			} catch (...) {
				bd = false;
			}
			if (ad != bd)
				return ad > bd;
			return a.path().filename() < b.path().filename();
		});
	} catch (...) {
		outEntries.clear();
		outError = "Error sorting directory entries.";
		return false;
	}

	return true;
}
/////////////////////////////////



/////////////////////////////////
// MusicVisualizerScene class implementation. This scene allows users to load music files, control playback, and visualize audio-reactive spawns based on the music's 
// spectrum analysis. It includes an ImGui interface for file browsing, playback controls, and spawn system configuration. The scene manages a music entity with a 
// CMusic component for audio playback and analysis, and a SpawnSystem for handling music-reactive spawns. The Update function processes music levels to trigger 
// spawns and renders the ImGui UI for user interaction.
MusicVisualizerScene::MusicVisualizerScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager): Scene(engine, entityManager), m_window(win) {}
/////////////////////////////////



/////////////////////////////////
// Destructor for the MusicVisualizerScene. It ensures that any dynamically allocated resources, such as the SpawnSystem, are properly released when the scene is 
// destroyed to prevent memory leaks.
MusicVisualizerScene::~MusicVisualizerScene() {
	delete m_spawnSystem;
	m_spawnSystem = nullptr;
}
/////////////////////////////////



/////////////////////////////////
// Update - the main update loop for the MusicVisualizerScene. It processes music playback state, updates audio-reactive spawns based on the current music level, 
// and renders the ImGui interface for music loading and playback controls. It also handles initialization of the current directory for the file browser and manages 
// the state of the spawn system and equalizer bars based on user interactions and music analysis.
void MusicVisualizerScene::Update(float deltaTime) {

	// Minimal update: process explosions and audio-reactive spawns similar to TileMapEditorScene
	// Pause visual updates when music is paused so effects stop on pause
	bool musicPaused = false;
	if (m_musicEntity) {
		if (auto musicCmp = m_musicEntity->GetComponent<CMusic>()) {
			musicPaused = (musicCmp->state == CMusic::State::Paused);
		}
	}

	if (!musicPaused) {
		UpdateExplosions();
	}

	// Get the current FPS
	m_fps = m_gameEngine.GetFPSCounter().GetFPS();

	// Ensure current dir is initialized - try to find a Music folder in common locations
	if (m_currentDir.empty()) {
		// First, try to get the actual Music folder location using Windows Shell API
		// This handles the case where the Music folder is redirected to another drive (e.g., D:\Music)
		PWSTR musicFolderPath = nullptr;
		if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Music, 0, nullptr, &musicFolderPath))) {
			std::filesystem::path musicPath(musicFolderPath);
			CoTaskMemFree(musicFolderPath); // Don't forget to free!

			std::error_code ec;
			if (std::filesystem::exists(musicPath, ec) && std::filesystem::is_directory(musicPath, ec)) {
				m_currentDir = musicPath;
			}
		}

		// If Shell API didn't work, fall back to checking common locations manually
		if (m_currentDir.empty()) {
			char* userProfileBuf = nullptr;
			size_t len = 0;
			bool foundUserProfile = (_dupenv_s(&userProfileBuf, &len, "USERPROFILE") == 0 && userProfileBuf != nullptr);

			if (foundUserProfile) {
				std::filesystem::path userPath(userProfileBuf);
				free(userProfileBuf);

				// List of fallback locations to check
				std::vector<std::filesystem::path> fallbackPaths = {
					userPath / "Downloads", // Downloads (people often have music here)
					userPath / "Desktop",	// Desktop
					userPath / "Documents", // Documents folder
					userPath				// User profile root
				};

				for (const auto& path : fallbackPaths) {
					std::error_code ec;
					if (std::filesystem::exists(path, ec) && std::filesystem::is_directory(path, ec)) {
						m_currentDir = path;
						break;
					}
				}
			}
		}

		// Final fallback to current working directory if nothing found
		if (m_currentDir.empty()) {
			m_currentDir = std::filesystem::current_path();
		}
	}

	// Draw ImGui UI for music loading and playback controls. This is done in Update so it is rendered within the main Music Visualizer, which makes more fucking sense.
	if (GImGui && GImGui->WithinFrameScope) {
		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
		ImGui::Begin("Music Visualizer", nullptr, ImGuiWindowFlags_None);
		ImGui::Text("Music Visualizer - FPS: %.1f", m_fps);

		// Music loader UI.. keep inside the Music Visualizer window so it is not placed in the default debug window
		ImGui::Separator();
		ImGui::Text("Music (assets)");

		// File browser + reactive spawner + playback controls moved to helpers
		ShowOpenFileBrowser();
		DrawAudioReactiveWindow();
		DrawPlaybackControls();

		ImGui::End();
	}

	// Update spawn system - this actually triggers the spawning based on music level
	// Ensure spawn system exists (initialize here so it works even if ImGui isn't rendering)
	if (!m_spawnSystem) {
		m_spawnSystem = new Spawn::SpawnSystem(&m_entityManager, &m_window);
		m_spawnSystem->LoadDefault();
	}

	if (m_spawnSystem) {
		float levelForSpawn = 0.0f;
		if (m_musicEntity) {
			if (auto ms = m_entityManager.GetMusicSystem()) {
				
				// If UI requested a restart (Play pressed after track end), seek to start before processing
				if (m_requestRestart) {
					ms->Seek(m_musicEntity->GetId(), 0.0f);
					m_requestRestart = false;
				}
				ms->Process();
               levelForSpawn = ms->GetLevel(m_musicEntity->GetId());
                // Update equalizer bars from latest spectrum (if enabled and available)
			   if (m_EqualizerActive) {
					std::vector<float> bands;
					auto& pool = m_entityManager.GetEntities(EntityType::Equalizer);
					if (pool.empty()) {
						InitializeEqualizerBars(static_cast<size_t>(m_visualBarCount));
					}

					if (musicPaused) {
						// When paused, decay bars to zero instead of continuing reactive updates.
						size_t poolSize = pool.size();
						if (poolSize == 0) poolSize = ms->GetSpectrumBandCount();
						if (poolSize == 0) poolSize = 8;
						bands.assign(poolSize, 0.0f);
						UpdateEqualizerBars(bands);
						m_lastSpectrumBands.clear();
					} else if (ms->GetSpectrum(m_musicEntity->GetId(), bands)) {
						// Try to get per-band spectrum data; cache it for continuity across brief analyzer gaps
						m_lastSpectrumBands = bands;
						UpdateEqualizerBars(bands);
					} else if (!m_lastSpectrumBands.empty()) {
						// If analyzer misses a frame, gently decay cached spectrum instead of hard-freezing it.
						float decay = std::clamp(deltaTime * 6.0f, 0.0f, 1.0f);
						for (float& v : m_lastSpectrumBands) {
							v = std::max(0.0f, v * (1.0f - decay));
						}
						UpdateEqualizerBars(m_lastSpectrumBands);
					} else {
						// No per-band spectrum available yet; fall back to overall level so bars still react
						float lvl = ms->GetLevel(m_musicEntity->GetId());
						size_t poolSize = pool.size();
						if (poolSize == 0) poolSize = ms->GetSpectrumBandCount();
						if (poolSize == 0) poolSize = 8;
						bands.assign(poolSize, lvl);
						UpdateEqualizerBars(bands);
					}
				} else {
					// If equalizer disabled, ensure bars are hidden
					HideEqualizerBars();
				}
			}
		}
		// Inform spawn system which music entity to use for audio-reactive patterns
		if (m_musicEntity) m_spawnSystem->SetMusicEntityId(m_musicEntity->GetId());

		// Apply reactivity controls to spawn system before updating
		m_spawnSystem->SetReactivitySensitivity(m_reactivitySensitivity);
		m_spawnSystem->SetVelocityScale(m_velocityScale);
		m_spawnSystem->SetBurstIntensity(m_burstIntensity);
		m_spawnSystem->SetBeatThreshold(m_beatThreshold);

		m_spawnSystem->Update(deltaTime, levelForSpawn);
	}

	// Display music status (loaded file or errors)
	if (!m_musicStatus.empty())
		ImGui::TextUnformatted(m_musicStatus.c_str());

	// simple input
	ProcessInput();
}
/////////////////////////////////



/////////////////////////////////
// Render - the main render function for the MusicVisualizerScene. This function is responsible for rendering any custom visuals for the scene, but in this case, 
// we are doing all of our rendering through ImGui and the SpawnSystem's CShape components, so this function can be minimal. We will render the grid as part of the 
// debug overlay, and the music-reactive visuals (explosions) are rendered via their CShape components in the main render loop of the engine, so we don't need to do 
// anything special here for them. However, if we wanted to add any additional custom rendering specific to the music visualizer in the future, this would be 
// the place to do it.
void MusicVisualizerScene::Render() {}
/////////////////////////////////



// DoAction - this function is meant to handle any specific actions or updates that need to occur in the scene, but for the MusicVisualizerScene, we are handling all 
// of our updates in the Update function, and we don't have any specific actions that need to be triggered separately, so we can just leave this empty for now. 
// If we wanted to add any special behavior that should be triggered on a timer or in response to certain conditions, we could implement that here.
void MusicVisualizerScene::DoAction() {}
/////////////////////////////////



/////////////////////////////////
// RenderDebugOverlay - this function is responsible for rendering any debug visuals for the scene. For the MusicVisualizerScene, we will use this to render a grid overlay 
// on the window, which can help visualize the space and add a nice aesthetic for the music visualizer. We will set the view to the default view to ensure the grid is aligned 
// with the window coordinates, then draw the grid and restore the previous view. This way, the grid will always be rendered in screen space and won't be affected by any 
// camera transformations that might be applied to other entities in the scene.
void MusicVisualizerScene::RenderDebugOverlay() {
	// Draw grid only
	sf::View prevView = m_window.getView();
	sf::View view = m_window.getDefaultView();
	m_window.setView(view);
	DrawGrid();
	m_window.setView(prevView);
}
/////////////////////////////////



/////////////////////////////////
// HandleEvent - this function is meant to handle any SFML events that are relevant to the scene, such as keyboard input, mouse input, window events, etc. However, for the 
// MusicVisualizerScene, we are handling user input in a more immediate mode style within the Update function (e.g., checking key states for the Escape key to close the window), 
// and we don't have any specific event-based interactions that we need to handle separately,
void MusicVisualizerScene::HandleEvent(const std::optional<sf::Event>& event) {}
/////////////////////////////////



// OnEnter and OnExit - these functions are called when the scene is entered or exited, respectively. For the MusicVisualizerScene,
// we now keep spatial audio enabled so users can experiment with 3D positioning of the music through the GUI controls.
void MusicVisualizerScene::OnEnter() {
	// Initialize the listener position to the screen center so 3D sounds are positioned correctly from the start
	Vec2 listenerPos(m_window.getSize().x / 2.0f, m_window.getSize().y / 2.0f);
	if (m_entityManager.GetSoundSystem()) {
		m_entityManager.GetSoundSystem()->SetListenerPosition(listenerPos);
	}
}
/////////////////////////////////



/////////////////////////////////
// OnExit 
void MusicVisualizerScene::OnExit() 
{
	if (m_musicEntity) {
		m_entityManager.KillEntity(m_musicEntity);
		m_entityManager.Update(0.0f);
		m_musicEntity = nullptr;
	}
}



/////////////////////////////////
// OnWindowResized
void MusicVisualizerScene::OnWindowResized(sf::Vector2u newSize) {
	sf::View view;
	view.setCenter(sf::Vector2f(newSize.x * 0.5f, newSize.y * 0.5f));
	view.setSize(sf::Vector2f(newSize.x, newSize.y));
	m_window.setView(view);
}
/////////////////////////////////



/////////////////////////////////
// LoadResources and UnloadResources - these functions are meant to handle the loading and unloading of any resources that the scene needs, such as textures, sounds, music, etc. 
// However, for the MusicVisualizerScene, we are loading music files dynamically based on user selection through the ImGui file browser, and we don't have any specific resources 
// that we need to load or unload at the scene level,
void MusicVisualizerScene::LoadResources() { m_isLoaded = true; }
void MusicVisualizerScene::UnloadResources() {}
/////////////////////////////////



/////////////////////////////////
// InitializeGame - this function is responsible for initializing the game state for the scene when it is first created. For the MusicVisualizerScene, we will set up a tile map that 
// covers the entire window, which we can use for visual effects or as a background grid. We will calculate the number of columns and rows needed based on the window size and a defined 
// tile size, and then create a TileMap instance with those dimensions. This will allow us to easily draw a grid overlay in the RenderDebugOverlay function and potentially use the 
// tile map for other visual effects in the future.
void MusicVisualizerScene::InitialiseGame(sf::Vector2u windowSize) {
	const float tileSize = 32.0f;
	int cols = static_cast<int>(windowSize.x / static_cast<unsigned int>(tileSize)) + 2;
	int rows = static_cast<int>(windowSize.y / static_cast<unsigned int>(tileSize)) + 2;
	m_tileMap = TileMap(cols, rows, tileSize);
}
/////////////////////////////////



/////////////////////////////////
// DrawGrid - this function is responsible for drawing a grid overlay on the window based on the tile map we set up in InitializeGame. We will iterate through each tile in the tile map 
// and enqueue rectangle outlines to the render queue for it using the engine's centralized rendering system. The rectangles will be transparent with a light outline color to create a 
// subtle grid effect that doesn't overpower the visuals of the music visualizer. We also check if the tile map has valid dimensions before attempting to draw to avoid unnecessary processing.
void MusicVisualizerScene::DrawGrid() {
	if (m_tileMap.width <= 0 || m_tileMap.height <= 0)
		return;

	// Clear temporary storage from previous frame
	m_tempGridShapes.clear();
	m_nextTempShapeId = 0;

	// Create and enqueue grid rectangles
	for (int y = 0; y < m_tileMap.height; ++y) {
		for (int x = 0; x < m_tileMap.width; ++x) {
			auto outline = std::make_shared<sf::RectangleShape>(sf::Vector2f(m_tileMap.tileSize, m_tileMap.tileSize));
			outline->setPosition(sf::Vector2f(x * m_tileMap.tileSize, y * m_tileMap.tileSize));
			outline->setFillColor(sf::Color::Transparent);
			outline->setOutlineColor(sf::Color(200, 200, 200, 60));
			outline->setOutlineThickness(1.0f);

			// Store the shape to keep it alive through the frame
			m_tempGridShapes.push_back(outline);

			// Enqueue to the engine render queue at depth 5 (behind most other elements)
			GetEngineRenderQueue().Enqueue(outline.get(), 5);
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// ProcessInput - this function is responsible for processing user input for the scene. For the MusicVisualizerScene, we will check for the Escape key to allow the user to close the 
// window and exit the application. This provides a simple way for users to exit the music visualizer without needing to interact with the window controls, which can be especially 
// useful if the visualizer is running in fullscreen mode.
void MusicVisualizerScene::ProcessInput() {
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
		m_gameEngine.ChangeScene("MainMenu");
	}
}
/////////////////////////////////



/////////////////////////////////
// LoadMusicFromPath - this function is responsible for loading a music file from a given file path and setting it up for playback in the scene. It first checks if there is an existing 
// music entity and destroys it to ensure that only one music track is playing at a time. Then, it creates a new entity and adds a CMusic component to it with the specified file path, 
// default volume, loop setting based on the current UI state, and starts playback immediately.
void MusicVisualizerScene::LoadMusicFromPath(const std::string& path) {
	// Kill any existing music entity to ensure we only have one music playing at a time. We also call Update after killing the entity to ensure it is fully removed before
	// we create a new one, which can help prevent issues with the MusicSystem still trying to process the old entity.
	if (m_musicEntity) {
		m_entityManager.KillEntity(m_musicEntity);
		m_entityManager.Update(0.0f);
		m_musicEntity = nullptr;
	}
	Entity* musicEntity = m_entityManager.AddEntity(EntityType::Default);

	// Guard: No entity created, return early
	if (!musicEntity)
		return;

	// Get here? then we have an entity to work with, so add a CMusic component with the given path and default settings (volume 70, start playing immediately).
	// Use the current m_loopEnabled setting from the UI for the loop state.
	auto* musicComponent = musicEntity->AddComponent<CMusic>(path, 70.f, m_loopEnabled, true);
	musicComponent->state = CMusic::State::Playing;
	musicComponent->loop = m_loopEnabled; // Respect current UI loop setting

	// Calculate the actual screen center and initialize music position to listener position
	m_musicX = m_window.getSize().x / 2.0f;
	m_musicY = m_window.getSize().y / 2.0f;

	// Add CTransform so music entity can have a 3D position for spatial audio, starting at listener position
	auto* transform = musicEntity->AddComponent<CTransform>(Vec2(m_musicX, m_musicY), Vec2::Zero);

	// Initialize GUI member variables from component defaults
	if (auto musicCmp = musicEntity->GetComponent<CMusic>()) {
		m_musicMinDistance = musicCmp->m_3DMinDistance;
		m_musicMaxDistance = musicCmp->m_3DMaxDistance;
	}

	m_musicEntity = musicEntity;
	m_entityManager.ProcessPending();

	// Process the music system immediately to start playing the music and have the analysis buffer available right away for the audio-reactive spawning.
	// This ensures that as soon as we load a music file, it starts playing and we can see the visual effects without delay.
	if (auto ms = m_entityManager.GetMusicSystem())
		ms->Process();
	m_musicStatus = std::string("Loaded: ") + path;

    // Initialize equalizer bars when loading music so we have pre-allocated bar entities
	if (auto ms2 = m_entityManager.GetMusicSystem()) {
		// Create visual pool using current visual bar count (independent of spectrum bands)
		InitializeEqualizerBars(static_cast<size_t>(m_visualBarCount));
	}
}
/////////////////////////////////



/////////////////////////////////
// ToggleTileAt - this function is meant to toggle the solidity of a tile at the given tile coordinates, which could affect player movement or interactions with the environment. 
// However, for the MusicVisualizerScene, we don't have any solid tiles or collision, so this function can be left empty for now.
void MusicVisualizerScene::ToggleTileAt(int tx, int ty, bool setSolid) {}
/////////////////////////////////



/////////////////////////////////
// UpdateExplosions - this function is responsible for updating the state of explosion entities in the scene. It iterates through all entities and checks for those that are of type Explosion. 
// For each explosion, it calculates how long it has been alive and if it has exceeded a certain duration (e.g., 1200 milliseconds), it destroys the entity to remove it from the scene.
void MusicVisualizerScene::UpdateExplosions() {
	m_explosionCount = 0;
	auto now = std::chrono::high_resolution_clock::now();

	// Iterate through all entities and process those that are explosions
	for (auto& entity : m_entityManager.GetEntities()) {
		if (entity->GetType() == EntityType::Explosion) {
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
				now - entity->m_creationTime); // Calculate how long the explosion has been alive in milliseconds

			// If the explosion has been alive for more than 9800 milliseconds, destroy it to remove it from the scene
			if (elapsed.count() >9800) {
				entity->Destroy();
			} else { // Otherwise, update its visual properties to create a fading effect as it ages
				++m_explosionCount;
                float fadeProgress = static_cast<float>(elapsed.count()) / 9800.0f;
				const int maxAlpha = 60;
				int newAlpha = static_cast<int>(maxAlpha * (1.0f - fadeProgress));

				auto shape = entity->GetComponent<CShape>();


				// If the entity has a CShape component, update its visual properties to create fade/expansion effects.
				if (shape) {
					if (auto* explosion = dynamic_cast<CExplosion*>(shape)) {
						explosion->SetRadius(explosion->GetRadius() * 1.004f);
						sf::Color currentColor = explosion->GetColor();
						explosion->SetColor(static_cast<float>(currentColor.r), static_cast<float>(currentColor.g),
											static_cast<float>(currentColor.b), newAlpha);
					} else if (auto* rect = dynamic_cast<CRectangle*>(shape)) {
						float w = rect->GetWidth() * 1.003f;
						float h = rect->GetHeight() * 1.003f;
						rect->SetSize(w, h);
						sf::Color currentColor = rect->GetColor();
						rect->SetColor(static_cast<float>(currentColor.r), static_cast<float>(currentColor.g),
								  static_cast<float>(currentColor.b), newAlpha);
					}
				}
			}
		}
	}
}
/////////////////////////////////