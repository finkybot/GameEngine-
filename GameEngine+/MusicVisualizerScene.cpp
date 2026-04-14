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
#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>
#include <imgui/imgui_internal.h>
#include <filesystem>
#include <ShlObj.h>  // Windows Shell API for known folder paths


// Spawn a single audio-reactive explosion entity with random size, color, and position.
// Called when audio level exceeds threshold and cooldown has elapsed.
void MusicVisualizerScene::SpawnAudioReactiveExplosion(bool resetSpawnTimer) {
	Entity* spawnedEntity = m_entityManager.addEntity(EntityType::Explosion);
	if (!spawnedEntity) return;

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
	if (resetSpawnTimer) m_spawnTimer = 0.0f;
}


// Similar to SpawnCircularExplosion but size scales with the provided music level (0.0 - 1.0 expected range)
void MusicVisualizerScene::SpawnCircularExplosionByLevel(float level, bool resetSpawnTimer) {
	Entity* spawnedEntity = m_entityManager.addEntity(EntityType::Explosion);
	if (!spawnedEntity) return;

	// Clamp level
	if (level < 0.0f) level = 0.0f;
	if (level > 1.0f) level = 1.0f;

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
	auto clamp8 = [](int v) { if (v < 0) return 0; if (v > 255) return 255; return v; };
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
	if (m_circularAngle > 3.14159265f * 2.0f) m_circularAngle -= 3.14159265f * 2.0f;

	m_entityManager.ProcessPending();
	if (resetSpawnTimer) m_spawnTimer = 0.0f;
}


// Spawn explosions in a deterministic circular pattern around screen center.
// Each spawn advances the angle by m_circularSpeed and places the explosion on the circle of radius m_circularRadius.
void MusicVisualizerScene::SpawnCircularExplosion(bool resetSpawnTimer) {
	Entity* spawnedEntity = m_entityManager.addEntity(EntityType::Explosion);
	if (!spawnedEntity) return;

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
	if (m_circularAngle > 3.14159265f * 2.0f) m_circularAngle -= 3.14159265f * 2.0f;

	m_entityManager.ProcessPending();
	if (resetSpawnTimer) m_spawnTimer = 0.0f;
}


// Draw the audio reactive spawn controls and handle spawning entities based on music analysis
void MusicVisualizerScene::DrawAudioReactiveWindow() {
	if (!(GImGui && GImGui->WithinFrameScope)) return;

	// Position the Audio Reactive window in the bottom-right corner with a fixed size
	ImVec2 winSize(340, 400);
	ImVec2 pos((float)m_window.getSize().x - winSize.x - 10.0f, (float)m_window.getSize().y - winSize.y - 10.0f);
	ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);
	ImGui::Begin("Audio Reactive", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

	// Enable/disable spawn system
	bool spawnEnabled = m_spawnSystem ? m_spawnSystem->IsEnabled() : false;
	if (ImGui::Checkbox("Enable Reactive Spawn", &spawnEnabled)) {
		if (m_spawnSystem) m_spawnSystem->SetEnabled(spawnEnabled);
	}

	// If we have at least one spawner config, allow editing the first one
	if (m_spawnSystem && !m_spawnSystem->GetConfigs().empty()) {
		auto& spawnConfigs = m_spawnSystem->GetConfigsMutable();

		// Spawn type selector
		const char* typeItems[] = { "Burst", "Continuous", "Periodic" };
		int typeIdx = static_cast<int>(spawnConfigs[0].type);
		if (ImGui::Combo("Spawn Type", &typeIdx, typeItems, IM_ARRAYSIZE(typeItems))) {
			spawnConfigs[0].type = static_cast<Spawn::Type>(typeIdx);
		}

		// Pattern selector (all new patterns)
		const char* patternItems[] = {
			"Random",
			"Circular",
			"Level-scaled Circular",
			"Spiral",
			"Firework",
			"Figure-8",
			"Wave",
			"Multi-Ring",
			"Starburst",
			"Helix"
		};
		int patternIdx = static_cast<int>(spawnConfigs[0].pattern);
		if (ImGui::Combo("Spawn Pattern", &patternIdx, patternItems, IM_ARRAYSIZE(patternItems))) {
			spawnConfigs[0].pattern = static_cast<Spawn::Pattern>(patternIdx);
		}

		// Threshold (used by Burst/Continuous)
		if (spawnConfigs[0].type != Spawn::Type::Periodic) {
			ImGui::SliderFloat("Threshold", &spawnConfigs[0].threshold, 0.0001f, 0.5f, "%.4f");
		} else {
			ImGui::TextColored(ImVec4(0.7f,0.7f,0.7f,1), "Threshold: ignored (Periodic)");
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
		bool needsRotation = (pat == Spawn::Pattern::Circular || pat == Spawn::Pattern::LevelScaledCircular || pat == Spawn::Pattern::Spiral || pat == Spawn::Pattern::Figure8 || pat == Spawn::Pattern::MultiRing || pat == Spawn::Pattern::Helix);
		if (needsRotation) ImGui::SliderFloat("Rotation Speed", &spawnConfigs[0].circularSpeed, 0.01f, 1.0f, "%.2f");
		if (pat == Spawn::Pattern::Spiral) ImGui::SliderFloat("Spiral Expansion", &spawnConfigs[0].spiralExpansion, 0.1f, 20.0f, "%.1f");
		if (pat == Spawn::Pattern::MultiRing) ImGui::SliderInt("Ring Count", &spawnConfigs[0].ringCount, 2, 8);

		ImGui::Separator();
		ImGui::Text("Spawners: %d", (int)m_spawnSystem->GetConfigs().size());
	} else {
		ImGui::TextUnformatted("No spawners configured.");
	}

	// Show current music level for reference
	float level = 0.0f; bool hasBuffer = false;
	if (m_musicEntity) {
		if (auto ms = m_entityManager.GetMusicSystem()) { ms->Process(); level = ms->GetLevel(m_musicEntity->GetId()); hasBuffer = ms->HasAnalysisBuffer(m_musicEntity->GetId()); }
	}
	ImGui::Text("Level: %.4f", level); ImGui::SameLine(); ImGui::TextUnformatted(hasBuffer ? "(analyzing)" : "(no analysis)");

	ImGui::End();
}


// Draw playback controls for the currently loaded music
void MusicVisualizerScene::DrawPlaybackControls() {
	// Display music status and controls
	if (!m_musicStatus.empty()) ImGui::TextUnformatted(m_musicStatus.c_str());

	// If a music entity exists
	if (m_musicEntity) {
		// Get the CMusic component from the music entity
		if (auto musicCmp = m_musicEntity->GetComponent<CMusic>()) {
			// Display the current state of the music
			const char* stateText = "Unknown"; // unknown state
			switch (musicCmp->state) {
				case CMusic::State::Playing: stateText = "Playing";     break;
				case CMusic::State::Paused: stateText = "Paused";       break;
				case CMusic::State::Stopped: stateText = "Stopped";     break;
			}
			// set the text to show the current state of the music
			ImGui::Text("State: %s", stateText);

           // Playback control buttons: change the music state and process immediately
            if (ImGui::Button("Play")) {
				musicCmp->state = CMusic::State::Playing;
				std::cout << "[MusicVisualizer] Play pressed for entity " << (m_musicEntity ? m_musicEntity->GetId() : 0) << std::endl;
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
			if (ImGui::Button("Pause")) { musicCmp->state = CMusic::State::Paused; if (auto musicSys = m_entityManager.GetMusicSystem()) musicSys->Process(); } // Pause music
			ImGui::SameLine();
			if (ImGui::Button("Stop")) { musicCmp->state = CMusic::State::Stopped; if (auto musicSys = m_entityManager.GetMusicSystem()) musicSys->Process(); } // Stop music
			
			// Volume slider: need to process any change in order for it to apply immediately
			float vol = musicCmp->volume;
			if (ImGui::SliderFloat("Volume", &vol, 0.0f, 100.0f, "%.0f")) { musicCmp->volume = vol; if (auto musicSys = m_entityManager.GetMusicSystem()) musicSys->Process(); }      // Adjust volume
		}
	}

	// Loop toggle and playhead / seek
	if (m_musicEntity) {
		if (auto musicSys = m_entityManager.GetMusicSystem()) {
			m_playhead = musicSys->GetPlayingOffset(m_musicEntity->GetId());
			m_duration = musicSys->GetDuration(m_musicEntity->GetId());
		}

		// Sync m_loopEnabled with component state first (in case it changed externally)
		if (auto musicCmp = m_musicEntity->GetComponent<CMusic>()) {
			m_loopEnabled = musicCmp->loop;
		}

		// Loop checkbox - process immediately when changed
		if (ImGui::Checkbox("Loop", &m_loopEnabled)) {
			if (auto musicCmp = m_musicEntity->GetComponent<CMusic>()) {
				musicCmp->loop = m_loopEnabled;
				// Process immediately to update sf::Music looping state
				if (auto musicSys = m_entityManager.GetMusicSystem()) musicSys->Process();
			}
		}

		// Playhead slider (seek)
		if (m_duration > 0.0f) {
			float newPos = m_playhead;
			ImGui::SliderFloat("Playhead (s)", &newPos, 0.0f, m_duration, "%.2f");

			// Detect drag start/end using IsItemActive
			bool sliderActive = ImGui::IsItemActive();

			// On drag start, if music is playing, pause it and remember to resume on drag end. On drag end, if we paused for the drag, resume playback.
			if (sliderActive && !m_playheadActive) {
				// Drag just started - record playing state and pause (I want to stop the sound sample from playing while dragging the playhead, as it 
				// will likely repeat the same sample over and over (sounds like game crashes!!!! anyone)
				if (auto cm = m_musicEntity->GetComponent<CMusic>()) {
					m_wasPlayingBeforeSeek = (cm->state == CMusic::State::Playing); // record the music playing state before seek
					// if it was playing, pause it while dragging the playhead to prevent repeated sound samples during seek (which sound like ass)
					if (m_wasPlayingBeforeSeek) {
						cm->state = CMusic::State::Paused;
						if (auto musicSys = m_entityManager.GetMusicSystem()) musicSys->Process();
					}
				}
			}
			// otherwise check if the slider is not active but play ahead is
			else if (!sliderActive && m_playheadActive) {
				if (m_wasPlayingBeforeSeek) {
					if (auto musCmp = m_musicEntity->GetComponent<CMusic>()) {
						musCmp->state = CMusic::State::Playing;
                        if (auto musicSys = m_entityManager.GetMusicSystem()) musicSys->Process();
					}
				}
			}
			m_playheadActive = sliderActive; // set the playhead active state, true if we are currently dragging the playhead, and false after we let go, I'm updating this at the end that way I can check the previous state when we detect the transition from not active to active (drag start) and from active to not active (drag end) to implement the pause on drag behavior, lots and lots of words, blame copilot it took over half of this comment, I just wanted to explain the reasoning behind the pause on drag behavior and how we are detecting the drag start and end using the slider active state and the m_playheadActive member variable to track whether we are currently dragging the playhead or not, which is important for implementing the pause on drag behavior to prevent repeated sound samples during seek which sound like, well, like, did you actually read this far???????

			ImGui::SameLine();
			ImGui::Text("/ %.2fs", m_duration);

			// Seek while dragging (or on any change)
			if (newPos != m_playhead) {
                if (auto musicSys = m_entityManager.GetMusicSystem()) musicSys->Seek(m_musicEntity->GetId(), newPos);
				m_playhead = newPos;
			}
		} else {
			ImGui::TextUnformatted("Playhead: n/a");
		}
	}

}


// Draw the file browser UI for opening audio files.I'm using ImGui, When a music file is selected, it will load the music into the scene
// and create a music entity with a CMusic component to play the selected music.
// Forward declarations of helper functions defined below
void MusicVisualizerScene::ShowOpenFileBrowser() {
	ImGui::SameLine(); // Keep the "Browse..." button on the same line as the previous UI elements

	// Try and create abutton to open the file browser popup
	if (ImGui::Button("Browse...")) {
		m_showOpenDialog = true; ImGui::OpenPopup("Open Audio File");
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

        // If we haven't refreshed the current directory yet, or if the current directory has changed since the last refresh,
		// normalize to an absolute path before comparing. This avoids subtle mismatches due to relative vs absolute or trailing slash
		// differences that can prevent a refresh when the user navigates into a directory.
		std::filesystem::path normCur;
		try {
			normCur = std::filesystem::absolute(m_currentDir);
		} catch(...) { normCur = m_currentDir; }

		if (entries.empty() || lastRefreshedDir != normCur) {
			if (!RefreshDirectoryListing(normCur, entries, refreshError, skippedCount, showNonAudio)) {
				// on failure entries will be empty and refreshError contains the message
			}
			lastRefreshedDir = normCur;
			// ensure m_currentDir stores the normalized value too
			m_currentDir = normCur;
		}

		// Drive selection combo box: On Windows (I'm not planning this for other platforms but....), Users can select different drives (C:\, D:\, etc.) so populate a list of available drives 
		// and show it in a combo box. When the user selects a drive, change the current directory to the given drive and refresh the entries.
		std::vector<std::string> drives;

		// Check for drives and add them to the list if they exist. Using std::filesystem::exists to check if the root of the drive exists
		for (char d = 'A'; d <= 'Z'; ++d) {
			std::string root; root.push_back(d); root += ":\\";
			std::error_code errorCode; 
			if (std::filesystem::exists(root, errorCode)) drives.push_back(root);
		}

		// If we have any drives, show the combo box for drive selection and refresh the directory listing.
		static int selDrive = -1;
		if (!drives.empty()) {
			if (selDrive < 0 || selDrive >= (int)drives.size()) selDrive = 0;
			std::string items;
			for (size_t i = 0; i < drives.size(); ++i) { items += drives[i]; items.push_back('\0'); }
			if (ImGui::Combo("Drive", &selDrive, items.c_str())) { m_currentDir = drives[selDrive]; RefreshDirectoryListing(m_currentDir, entries, refreshError, skippedCount, showNonAudio); lastRefreshedDir = m_currentDir; }
		}

		// Checkbox to toggle showing non-audio files. When toggled, it will refresh the directory listing to apply the new filter.
		ImGui::SameLine(); ImGui::Checkbox("Show non-audio files", &showNonAudio);

		// Search box to filter filenames shown in the file browser
		static char searchBuf[256] = "";
		ImGui::SameLine(); ImGui::PushItemWidth(200);
		if (ImGui::InputText("Search", searchBuf, sizeof(searchBuf))) {
			/* user typed, we'll filter display */
		}
		ImGui::PopItemWidth();
		ImGui::SameLine(); if (ImGui::Button("Clear")) { searchBuf[0] = '\0'; }

		ImGui::Text("Current folder: %s", m_currentDir.string().c_str());
		if (ImGui::Button("Up") && m_currentDir.has_parent_path()) 
		{ 
			m_currentDir = m_currentDir.parent_path(); 
			RefreshDirectoryListing(m_currentDir, entries, refreshError, skippedCount, showNonAudio); lastRefreshedDir = m_currentDir; 
		}
		// Refresh button to manually refresh the directory listing.
		ImGui::SameLine(); if (ImGui::Button("Refresh")) { RefreshDirectoryListing(m_currentDir, entries, refreshError, skippedCount, showNonAudio); lastRefreshedDir = m_currentDir; }

		// Display any errors that occur during directory reading or refreshing, as well as the count of skipped entries due to permissions or other issues. Finally, show the count of items being displayed.
		if (!refreshError.empty()) { ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "%s", refreshError.c_str()); }
		if (skippedCount > 0) { ImGui::TextColored(ImVec4(1,0.7f,0,1), "Skipped %d entries (permissions/special chars)", skippedCount); }
		// We'll show the filtered count below after rendering the list

		// Child region to display the list of files and directories. Each entry is selectable, and double-clicking a directory will navigate into it, while double-clicking a file will select it for loading.
		ImGui::BeginChild("file_list", ImVec2(600,300), true);
		// Track how many entries we actually display after applying the search filter
		int displayedCount = 0;
		std::string queryLower;

		// Convert search query to lowercase for case-insensitive comparison. Using try/catch to handle any issues with the search string
		try { 
			queryLower = std::string(searchBuf); 
		} catch(...) { 
			queryLower.clear(); 
		}


		for (auto &ent : entries) {
			// Apply search filter (case-insensitive substring). If empty, show all.
			std::string nameTry;
			try { nameTry = ent.path().filename().string(); } catch(...) { nameTry.clear(); }
			
			std::string nameLower = nameTry;
			for (auto &c : nameLower) c = (char)tolower((unsigned char)c);
			
			std::string qLower = queryLower;
			for (auto &c : qLower) c = (char)tolower((unsigned char)c);
			
			if (!qLower.empty() && nameLower.find(qLower) == std::string::npos) continue;
			std::string name; try { name = ent.path().filename().string(); } catch(...) { name = "<unreadable>"; }
			
			bool is_dir = false; std::error_code ec; try { is_dir = ent.is_directory(ec); } catch(...) { is_dir = false; }
			std::string label = is_dir ? (name + "/") : name;
			std::string entPathStr; try { entPathStr = ent.path().string(); } catch(...) { entPathStr.clear(); }
			bool selected = (!selectedPath.empty() && selectedPath == entPathStr);
			
			// Render the selectable entry. If it's selected, update the selectedPath. If it's a directory and we double-click it, navigate into it and refresh the listing. If it's a file and we double-click it, select it for loading.
			if (ImGui::Selectable(label.c_str(), selected)) {
				if (!entPathStr.empty()) selectedPath = entPathStr;
                if (is_dir) {
					// normalize directory we are entering to absolute form to avoid later mismatches
					try { m_currentDir = std::filesystem::absolute(ent.path()); } catch(...) { m_currentDir = ent.path(); }
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
				
				// Load a file immediately on double-click: if it's a valid selection; create an entity and add a CMusic component with the selected file, finally close the popup and reset browser state.
				if (!sel.empty()) { // not empty or directory
					// kill and music entity that might exist, don't want multiple music entities
                    if (m_musicEntity) { m_entityManager.KillEntity(m_musicEntity); m_entityManager.Update(0.0f); m_musicEntity = nullptr; }
                    
					// create a new music entity with a CMusic component for the selected file
                    Entity* musicEntity = m_entityManager.addEntity(EntityType::Default);
					// add the componenent and music if we have an entity
                    if (musicEntity) {
                        LoadMusicFromPath(sel);
                    }
                }
                ImGui::CloseCurrentPopup(); m_showOpenDialog = false; break;
			}
		}
		ImGui::EndChild();
		ImGui::Separator();
		ImGui::Text("Showing %d items (filtered: %d)", (int)entries.size(), displayedCount);

		// OK and Cancel buttons: OK will load the selected file, Cancel will just close the popup.
        if (ImGui::Button("OK") && !selectedPath.empty()) {
            std::string sel = selectedPath;
            if (m_musicEntity) { m_entityManager.KillEntity(m_musicEntity); m_entityManager.Update(0.0f); m_musicEntity = nullptr; }
            Entity* musicEntity = m_entityManager.addEntity(EntityType::Default);
            
			// add the componenent and music if we have an entity bla bla bla, same as the double-click handler (might be better to refactor this into a helper function)
            if (musicEntity) {
                auto* musicComponent = musicEntity->AddComponent<CMusic>(sel, 80.f, true, true);
                musicComponent->state = CMusic::State::Playing; musicComponent->loop = true; m_musicEntity = musicEntity;
                m_entityManager.ProcessPending(); if (auto ms = m_entityManager.GetMusicSystem()) ms->Process();
                m_musicStatus = std::string("Loaded: ") + sel; std::cout << m_musicStatus << std::endl;
            }
            ImGui::CloseCurrentPopup(); m_showOpenDialog = false;
        }
        ImGui::SameLine(); if (ImGui::Button("Cancel")) { ImGui::CloseCurrentPopup(); m_showOpenDialog = false; }
        ImGui::EndPopup();
    }
}

// Refresh directory listing helper moved out of ShowOpenFileBrowser to avoid lambda capture/lifetime issues.
bool MusicVisualizerScene::RefreshDirectoryListing(const std::filesystem::path& dir, std::vector<std::filesystem::directory_entry>& outEntries, std::string& outError, int& outSkipped, bool showNonAudio)
{
    outEntries.clear(); outError.clear(); outSkipped = 0;

    // Basic validation
    try {
        auto native = dir.native();
        if (native.empty()) { outError = "Directory path empty"; return false; }
        if (native.size() > 32768) { outError = "Directory path too long"; return false; }
    } catch (...) {
        outError = "Invalid directory path";
        return false;
    }

    std::filesystem::directory_options opts = std::filesystem::directory_options::skip_permission_denied | std::filesystem::directory_options::follow_directory_symlink;

    try {
        std::error_code dirEc;
        std::filesystem::directory_iterator it(dir, opts, dirEc);
        if (dirEc) { outError = std::string("Error opening directory: ") + dirEc.message(); return false; }

        for (auto &entry : it) {
            std::error_code ec_entry; bool is_dir = false;
            try { is_dir = entry.is_directory(ec_entry); } catch(...) { ++outSkipped; continue; }
            if (ec_entry) { ++outSkipped; continue; }
            if (is_dir) { outEntries.push_back(entry); continue; }
            if (showNonAudio) { outEntries.push_back(entry); continue; }
            std::string fileExt;
            try { fileExt = entry.path().extension().string(); } catch(...) { fileExt.clear(); }
            for (auto &c : fileExt) c = (char)tolower((unsigned char)c);
            if (fileExt == ".mp3" || fileExt == ".ogg" || fileExt == ".wav" || fileExt == ".flac") outEntries.push_back(entry);
        }
    } catch (const std::exception& ex) { outError = std::string("Error iterating directory: ") + ex.what(); outEntries.clear(); return false; } catch(...) { outError = "Unknown error iterating directory"; outEntries.clear(); return false; }

    try {
        std::sort(outEntries.begin(), outEntries.end(), [](auto const& a, auto const& b){
            std::error_code ea, eb; bool ad=false, bd=false;
            try { ad = std::filesystem::is_directory(a.path(), ea); } catch(...) { ad = false; }
            try { bd = std::filesystem::is_directory(b.path(), eb); } catch(...) { bd = false; }
            if (ad != bd) return ad > bd;
            return a.path().filename() < b.path().filename();
        });
    } catch(...) { outEntries.clear(); outError = "Error sorting directory entries."; return false; }

    return true;
}


// Constructor and destructor for the MusicVisualizerScene. The constructor takes references to the game engine, the render window, and the entity manager, 
// and initializes the base Scene class with the engine and entity manager. It also stores a reference to the render window for use in rendering the grid and other visuals. 
// The destructor is defaulted since we don't have any special cleanup needed for this scene at the moment.
MusicVisualizerScene::MusicVisualizerScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager) : Scene(engine, entityManager), m_window(win) {}
MusicVisualizerScene::~MusicVisualizerScene() { delete m_spawnSystem; m_spawnSystem = nullptr; }


// Main update function for the scene. This will handle processing of music-reactive spawns (explosions) based on the current music level, as well as rendering the ImGui UI for music loading and playback controls. 
// We also get the current FPS from the engine's FPS counter to display in the UI. Additionally, we ensure that the current directory is initialized so that the file browser has a starting point when opened. 
// Finally, we call ProcessInput to handle any user input for interactions within the scene.
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
					userPath / "Downloads",                     // Downloads (people often have music here)
					userPath / "Desktop",                       // Desktop
					userPath / "Documents",                     // Documents folder
					userPath                                    // User profile root
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
        ImGui::Begin("Music Visualizer", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
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
			}
        }
        m_spawnSystem->Update(deltaTime, levelForSpawn);
    }

    // Display music status (loaded file or errors)
    if (!m_musicStatus.empty()) ImGui::TextUnformatted(m_musicStatus.c_str());

    // simple input
    ProcessInput();
}


// For this scene, at the moment we will do all rendering in the ImGui UI, so the main Render function can be minimal or empty.
// We will render the grid as part of the debug overlay, and the music-reactive visuals (explosions) are rendered via their 
// CShape components in the main render loop of the engine, so we don't need to do anything special here for them. but we could 
// always add more custom rendering here if we wanted to do something more unique for the music visualizer in the future.
void MusicVisualizerScene::Render() {}


// No specific actions for this scene, but we need to implement the pure virtual function from the base Scene class, so we can just leave these empty for now.
void MusicVisualizerScene::DoAction() {}


// Render the debug overlay, which in this case will just be the grid. We will set the view to the default view to ensure the grid is 
// aligned with the window coordinates, then draw the grid and restore the previous view.
void MusicVisualizerScene::RenderDebugOverlay() {
    // Draw grid only
    sf::View prevView = m_window.getView();
    sf::View view = m_window.getDefaultView();
    m_window.setView(view);
    DrawGrid();
    m_window.setView(prevView);
}


// Handle events specific to this scene. For now, we don't have any special event handling, but we need to implement the pure virtual function 
// from the base Scene class, so we can just leave these empty for now. I could process keypresses here but I think it makes more sense to do that in the ProcessInput function which is called from Update, 
// that way we can easily check for key states and implement things like "hold key to do something" which would be more difficult to do in an event-based system.
void MusicVisualizerScene::HandleEvent(const std::optional<sf::Event>& event) {}


// OnEnter and OnExit functions for scene transitions. For this music visualizer scene, we don't have any special setup or teardown needed when entering or exiting the scene, 
// so we can just leave these empty for now. However, if we wanted to add any special effects or reset certain state when entering or exiting the scene, these would be the places to do that.
void MusicVisualizerScene::OnEnter() {}
void MusicVisualizerScene::OnExit() {}


// Guess what? LoadResources and UnloadResources functions. For this scene, we don't have any specific resources to load or unload, so we can just leave these empty for now.
void MusicVisualizerScene::LoadResources() { m_isLoaded = true; }
void MusicVisualizerScene::UnloadResources() { }


// Initialize the scene with the given window size. For the music visualizer, we will set up a tile map that covers the entire window, which we can use for visual effects or as a background grid.
void MusicVisualizerScene::InitializeGame(sf::Vector2u windowSize)
{
    const float tileSize = 32.0f;
    int cols = static_cast<int>(windowSize.x / static_cast<unsigned int>(tileSize)) + 2;
    int rows = static_cast<int>(windowSize.y / static_cast<unsigned int>(tileSize)) + 2;
    m_tileMap = TileMap(cols, rows, tileSize);
}


// Draw a grid based on the tile map. This will render a simple grid overlay on the window, which can help visualize the space and add a nice aesthetic for the music visualizer. We will draw rectangles 
// for each tile in the tile map, using a light color and some transparency for the outlines so it doesn't overpower the visuals. We also check if the tile map has valid dimensions before attempting to 
// draw to avoid unnecessary processing.
void MusicVisualizerScene::DrawGrid() {
    if (m_tileMap.width <= 0 || m_tileMap.height <= 0) return;
    for (int y = 0; y < m_tileMap.height; ++y) {
        for (int x = 0; x < m_tileMap.width; ++x) {
            sf::RectangleShape outline(sf::Vector2f(m_tileMap.tileSize, m_tileMap.tileSize));
            outline.setPosition(sf::Vector2f(x * m_tileMap.tileSize, y * m_tileMap.tileSize));
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineColor(sf::Color(200,200,200,60));
            outline.setOutlineThickness(1.0f);
            m_window.draw(outline);
        }
    }
}


// Process user input for the scene. For the music visualizer, we will just check for the Escape key to allow the user to close the window and exit the application.
void MusicVisualizerScene::ProcessInput() {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
        m_gameEngine.m_window.close();
    }
}


// Load music from the given file path. This will create a new music entity with a CMusic component to play the selected music. If there is already a music entity, it will
void MusicVisualizerScene::LoadMusicFromPath(const std::string& path) {
	// Kill any existing music entity to ensure we only have one music playing at a time. We also call Update after killing the entity to ensure it is fully removed before 
	// we create a new one, which can help prevent issues with the MusicSystem still trying to process the old entity.
	if (m_musicEntity) { m_entityManager.KillEntity(m_musicEntity); m_entityManager.Update(0.0f); m_musicEntity = nullptr; }
	Entity* musicEntity = m_entityManager.addEntity(EntityType::Default);

	// Guard: No entity created, return early
	if (!musicEntity) return;

	// Get here? then we have an entity to work with, so add a CMusic component with the given path and default settings (volume 80, start playing immediately). 
	// Use the current m_loopEnabled setting from the UI for the loop state.
	auto* musicComponent = musicEntity->AddComponent<CMusic>(path, 80.f, m_loopEnabled, true);
	musicComponent->state = CMusic::State::Playing;
	musicComponent->loop = m_loopEnabled;  // Respect current UI loop setting
	m_musicEntity = musicEntity;
	m_entityManager.ProcessPending();

	// Process the music system immediately to start playing the music and have the analysis buffer available right away for the audio-reactive spawning. 
	// This ensures that as soon as we load a music file, it starts playing and we can see the visual effects without delay.
	if (auto ms = m_entityManager.GetMusicSystem()) ms->Process();
	m_musicStatus = std::string("Loaded: ") + path;
}


// Toggle the solidity of a tile at the given tile coordinates. For the music visualizer, we don't have any solid tiles or collision, so this function can be left empty for now. H
// owever, if we wanted to add some interactive elements to the grid in the future, we could implement this function to toggle whether a tile is solid or not, which could affect 
// player movement or interactions with the environment. that would be a wierd addition, but for now we will just leave it as a placeholder.
void MusicVisualizerScene::ToggleTileAt(int tx, int ty, bool setSolid) {}


// Update explosions: This function will iterate through all entities in the scene and look for those that are of type Explosion. For each explosion entity, we will check how long 
// it has been since it was created, and if it has been more than 2900 milliseconds, we will destroy the entity to remove it from the scene. If it is still within the lifespan, 
// we will update its visual properties to create a fading effect as it ages. This includes increasing its radius slightly and reducing its alpha transparency over time to create 
// a nice visual effect for the music-reactive explosions.
void MusicVisualizerScene::UpdateExplosions() {
    m_explosionCount = 0;
    auto now = std::chrono::high_resolution_clock::now();
    for (auto& entity : m_entityManager.getEntities()) {
        if (entity->GetType() == EntityType::Explosion) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - entity->m_creationTime);
            if (elapsed.count() > 2900) {
                entity->Destroy();
            } else {
                ++m_explosionCount;
                float fadeProgress = static_cast<float>(elapsed.count()) / 2900.0f;
                int newAlpha = static_cast<int>(80 * (1.0f - fadeProgress));

                auto shape = entity->GetComponent<CShape>();
                
                if (shape) {
                    if (auto* explosion = dynamic_cast<CExplosion*>(shape)) {
                        float radiusDifference = explosion->GetRadius();
                        explosion->SetRadius(explosion->GetRadius() * 1.004f);
                        radiusDifference = explosion->GetRadius() - radiusDifference;
                        Vec2 explosionPosition = entity->GetComponent<CTransform>()->m_position;
                        entity->GetComponent<CTransform>()->m_position = Vec2(explosionPosition.x, explosionPosition.y);
                        sf::Color currentColor = explosion->GetColor();
                        explosion->SetColor(static_cast<float>(currentColor.r), static_cast<float>(currentColor.g), static_cast<float>(currentColor.b), newAlpha);
                    }
                }
            }
        }
    }
}
