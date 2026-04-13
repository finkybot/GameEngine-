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
#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>
#include <imgui/imgui_internal.h>
#include <filesystem>

void MusicVisualizerScene::DrawAudioReactiveWindow()
{
    if (!(GImGui && GImGui->WithinFrameScope)) return;
    ImVec2 winSize(300, 120);
    ImVec2 pos((float)m_window.getSize().x - winSize.x - 10.0f, (float)m_window.getSize().y - winSize.y - 10.0f);
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);
    ImGui::Begin("Audio Reactive", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    ImGui::Checkbox("Enable Reactive Spawn", &m_audioReactive);
    ImGui::SliderFloat("Threshold", &m_spawnThreshold, 0.0001f, 0.2f, "%.4f");
    ImGui::SliderFloat("Cooldown (s)", &m_spawnCooldown, 0.02f, 1.0f, "%.2f");

    float level = 0.0f;
    bool hasBuffer = false;
    bool musicPlaying = true;
    if (m_musicEntity) 
    {
        if (auto cm = m_musicEntity->GetComponent<CMusic>()) musicPlaying = (cm->state == CMusic::State::Playing);
        if (auto musicSystem = m_entityManager.GetMusicSystem()) 
        {
            musicSystem->Process();
            level = musicSystem->GetLevel(m_musicEntity->GetId());
            hasBuffer = musicSystem->HasAnalysisBuffer(m_musicEntity->GetId());
        }
    }
    ImGui::Text("Level: %.4f", level);
    ImGui::SameLine(); ImGui::TextUnformatted(hasBuffer ? "(analyzing)" : "(no analysis)");

    m_spawnTimer += ImGui::GetIO().DeltaTime;
    if (m_audioReactive && musicPlaying && level > m_spawnThreshold) {
        if (m_spawnTimer >= m_spawnCooldown) {
            Entity* spawnedEntity = m_entityManager.addEntity(EntityType::Explosion);
            if (spawnedEntity) 
            {
                float size = 6.0f + static_cast<float>(std::rand() % 36);
                auto circle = std::make_unique<CExplosion>(size);
                int r = 100 + (std::rand() % 156);
                int g = 100 + (std::rand() % 156);
                int b = 100 + (std::rand() % 156);
                circle->SetColor((float)r, (float)g, (float)b, 220);
                spawnedEntity->AddComponentPtr<CShape>(std::move(circle));
                float cx = static_cast<float>(m_window.getSize().x) * 0.5f;
                float cy = static_cast<float>(m_window.getSize().y) * 0.5f;
                float jitter = 250.0f;
                float x = cx + (static_cast<float>(std::rand() % (static_cast<int>(jitter * 2 + 1))) - jitter);
                float y = cy + (static_cast<float>(std::rand() % (static_cast<int>(jitter * 2 + 1))) - jitter);
                auto* t = spawnedEntity->AddComponent<CTransform>(Vec2(x, y), Vec2(0.0f, -40.0f - static_cast<float>(std::rand() % 120)));
                (void)t;
                m_entityManager.ProcessPending();
                m_spawnTimer = 0.0f;
            }
        }
    }

    ImGui::End();
}


// Draw playback controls for the currently loaded music
void MusicVisualizerScene::DrawPlaybackControls()
{
    // Display music status and controls
    if (!m_musicStatus.empty()) ImGui::TextUnformatted(m_musicStatus.c_str());

	// If a music entity exists
    if (m_musicEntity) 
    {
		// Get the CMusic component from the music entity
        if (auto musicCmp = m_musicEntity->GetComponent<CMusic>()) 
        {
			// Display the current state of the music
			const char* stateText = "Unknown"; // unknown state
            switch (musicCmp->state) 
            {
                case CMusic::State::Playing: stateText = "Playing";     break;
                case CMusic::State::Paused: stateText = "Paused";       break;
                case CMusic::State::Stopped: stateText = "Stopped";     break;
            }
			// set the text to show the current state of the music
            ImGui::Text("State: %s", stateText);

			// Playback control buttonns: When we click a button, it will change the music state , from play, pauses and so on, then process the music system to apply the change immediately
            if (ImGui::Button("Play")) { musicCmp->state = CMusic::State::Playing; if (auto musicSys = m_entityManager.GetMusicSystem()) musicSys->Process(); }                       // Play music
            ImGui::SameLine(); 
            if (ImGui::Button("Pause")) { musicCmp->state = CMusic::State::Paused; if (auto musicSys = m_entityManager.GetMusicSystem()) musicSys->Process(); }                       // Pause music
            ImGui::SameLine(); 
            if (ImGui::Button("Stop")) { musicCmp->state = CMusic::State::Stopped; if (auto musicSys = m_entityManager.GetMusicSystem()) musicSys->Process(); }                       // Stop music
            
            // Volume slider: need to process any change in order for it to apply immediately
            float vol = musicCmp->volume;
			if (ImGui::SliderFloat("Volume", &vol, 0.0f, 100.0f, "%.0f")) { musicCmp->volume = vol; if (auto musicSys = m_entityManager.GetMusicSystem()) musicSys->Process(); }      // Adjust volume
        }
    }
   
    // Loop toggle and playhead / seek
    if (m_musicEntity) 
    {
        if (auto musicSys = m_entityManager.GetMusicSystem()) 
        {
            m_playhead = musicSys->GetPlayingOffset(m_musicEntity->GetId());
            m_duration = musicSys->GetDuration(m_musicEntity->GetId());
        }

        ImGui::Checkbox("Loop", &m_loopEnabled);
        // Apply loop to CMusic component if present
        if (auto musicCmp = m_musicEntity->GetComponent<CMusic>()) musicCmp->loop = m_loopEnabled;

        // Playhead slider (seek)
        if (m_duration > 0.0f) 
        {
            float newPos = m_playhead;
            ImGui::SliderFloat("Playhead (s)", &newPos, 0.0f, m_duration, "%.2f");

            // Detect drag start/end using IsItemActive
            bool sliderActive = ImGui::IsItemActive();

			// On drag start, if music is playing, pause it and remember to resume on drag end. On drag end, if we paused for the drag, resume playback.
            if (sliderActive && !m_playheadActive) 
            {
				// Drag just started - record playing state and pause (I want to stop the sound sample from playing while dragging the playhead, as it 
                // will likely repeat the same sample over and over (sounds like game crashes!!!! anyone)
                if (auto cm = m_musicEntity->GetComponent<CMusic>()) 
                {
					m_wasPlayingBeforeSeek = (cm->state == CMusic::State::Playing); // record the music playing state before seek
					// if it was playing, pause it while dragging the playhead to prevent repeated sound samples during seek (which sound like ass)
                    if (m_wasPlayingBeforeSeek) 
                    {
                        cm->state = CMusic::State::Paused;
                        if (auto musicSys = m_entityManager.GetMusicSystem()) musicSys->Process();
                    }
                }
            }
            // otherwise check if the slider is not active but play ahead is
            else if (!sliderActive && m_playheadActive) 
            {
                if (m_wasPlayingBeforeSeek) 
                {
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
            if (newPos != m_playhead) 
            {
                if (auto musicSys = m_entityManager.GetMusicSystem()) musicSys->Seek(m_musicEntity->GetId(), newPos);
                m_playhead = newPos;
            }
        }
        else 
        {
            ImGui::TextUnformatted("Playhead: n/a");
        }
    }

}



// Draw the file browser UI for opening audio files.I'm using ImGui, When a music file is selected, it will load the music into the scene
// and create a music entity with a CMusic component to play the selected music.
// Forward declarations of helper functions defined below
void MusicVisualizerScene::ShowOpenFileBrowser()
{
	ImGui::SameLine(); // Keep the "Browse..." button on the same line as the previous UI elements

	// Try and create abutton to open the file browser popup
    if (ImGui::Button("Browse...")) 
    {
        m_showOpenDialog = true; ImGui::OpenPopup("Open Audio File");
    }

	// If the popup is open then display the file browser UI
    if (m_showOpenDialog && ImGui::BeginPopupModal("Open Audio File", NULL, ImGuiWindowFlags_AlwaysAutoResize)) 
    {
		// Static variables to hold the state of the file browser across frames
        static std::vector<std::filesystem::directory_entry> entries;
        static std::string selectedPath;
        static std::string refreshError;
        static int skippedCount = 0;
        static bool showNonAudio = false;

		// Lambda function to refresh the directory listing. This will read the contents of the current directory and populate the 'entries' vector with the directory entries. It also handles errors and counts skipped entries due to permissions or other issues.
        auto refresh = [&]() 
        {
			// Clear previous state
            entries.clear();
            refreshError.clear();
            skippedCount = 0;

            // Lets read the current directory and populate the entries vector.
			// I'm using std::filesystem to read the directory contents, and relying on try-catch block to handle any exceptions that may occur during this process 
            // (e.g., due to permissions issues or special characters in file names). 
            // We'll skip files with permissions issues via the 'skip_permission_denied' option so we don't throw an exception.
            try 
            {
                std::filesystem::directory_options opts = std::filesystem::directory_options::skip_permission_denied;
                
				// Iterate through the directory entries and populate the 'entries' vector. Filter based on show non-audio files or not.
                for (std::filesystem::directory_iterator iter(m_currentDir, opts); iter != std::filesystem::directory_iterator(); ++iter) 
                {
					// For each entry, we will check if it's a directory or an audio file (if showNonAudio is false).
                    try 
                    {
						// setup error code for is_directory check, if it fails we skip this entry and count it as skipped
                        std::error_code ec_entry;

                        // Get the directory entry
                        const auto& entry = *iter;

						// Check if it's a directory, if we get an error, skip and add to skipped count; if it's a directory, we always 
                        // add it to the entries list so we can navigate into it, then continue to the next entry
                        bool is_dir = false;
						try { is_dir = std::filesystem::is_directory(entry.path(), ec_entry); }	catch (...) { is_dir = false; }  
                        if (ec_entry) { ++skippedCount; continue; }
                        if (is_dir) { entries.push_back(entry); continue; } 

						// Not a directory, if showNonAudio is true we add it to the list, otherwise we only want audio files (mp3, ogg, wav, flac) so check
						// the file extension and as an entry if its music.... If we get any error during this process (e.g., permissions, special characters), 
                        // skip the entry and count it as skipped.
						if (showNonAudio) // showing all files, so just add it to the list
                        {
                            entries.push_back(entry);
                        } 
						else // otherwise check for audio file extensions and only add it if it matches
                        {
                            std::string fileExt;
                            try { fileExt = entry.path().extension().string(); } catch(...) { fileExt.clear(); }
                            for (auto &character : fileExt) character = (char)tolower((unsigned char)character);

                            // MUSIKA!!!!
                            if (fileExt == ".mp3" || fileExt == ".ogg" || fileExt == ".wav" || fileExt == ".flac") entries.push_back(entry); // OROROROROR
                        }
                    }
                    catch (...) { ++skippedCount; }
                }

				// Sort entries: directories first, then alphabetically. Using try/catch to handle potential errors via a lambda comparator.
                std::sort(entries.begin(), entries.end(), /* *** Setup sort lambda */ [](auto const& entryA, auto const& entryB)
                {
					// Setup bools and error codes for is_directory checks.
                    bool a_dir = false, b_dir = false; 
                    std::error_code ea, eb;

					// Check if entryA is a directory, if we get an error we treat it as not a directory
                    try { a_dir = std::filesystem::is_directory(entryA.path(), ea); } catch(...) { a_dir = false; }
                        
                    // Check if entryB is a directory, if we get an error we treat it as not a directory
                    try { b_dir = std::filesystem::is_directory(entryB.path(), eb); } catch(...) { b_dir = false; }
                        
					// If one is a directory and the other is not, the directory should come first
                    if (a_dir != b_dir) return a_dir > b_dir;

					// Both are the same type (both directories or both files), so sort alphabetically by filename. Errors during filename retrieval will be treated as empty filenames and naturally sort to the top (they are empty so....).
                    return entryA.path().filename() < entryB.path().filename();
					}/* end of sort lambda *** */ );
            } 
			// Catch any exceptions that occur during the directory reading and sorting process. If an exception occurs, we clear the entries and set an error message to be displayed in the UI.
            catch (const std::exception& ex) 
            { 
                entries.clear(); 
                refreshError = std::string("Error: ") + ex.what(); 
            }
			catch (...) // Last resort catch-all to prevent crashes due to unexpected errors
            { 
                entries.clear(); 
                refreshError = "Unknown error reading directory, something wicked this way comes."; 
            }
        };

		// If we don't have any entries loaded yet, then refresh to load the current directory contents
        if (entries.empty()) refresh();

		// Drive selection combo box: On Windows (I'm not planning this for other platforms but....), Users can select different drives (C:\, D:\, etc.) so populate a list of available drives 
        // and show it in a combo box. When the user selects a drive, change the current directory to the given drive and refresh the entries.
        std::vector<std::string> drives;
        
		// Check for drives and add them to the list if they exist. Using std::filesystem::exists to check if the root of the drive exists
        for (char d = 'A'; d <= 'Z'; ++d) 
        {
            std::string root; root.push_back(d); root += ":\\";
            std::error_code errorCode; 
            if (std::filesystem::exists(root, errorCode)) drives.push_back(root);
        }
		// If we have any drives, show the combo box for drive selection and refresh the directory listing.
        static int selDrive = -1;
        if (!drives.empty()) 
        {
            if (selDrive < 0 || selDrive >= (int)drives.size()) selDrive = 0;
            std::string items;
            for (size_t i = 0; i < drives.size(); ++i) { items += drives[i]; items.push_back('\0'); }
            if (ImGui::Combo("Drive", &selDrive, items.c_str())) { m_currentDir = drives[selDrive]; refresh(); }
        }
        
		// Checkbox to toggle showing non-audio files. When toggled, it will refresh the directory listing to apply the new filter.
        ImGui::SameLine(); ImGui::Checkbox("Show non-audio files", &showNonAudio);
        ImGui::Text("Current folder: %s", m_currentDir.string().c_str());
        if (ImGui::Button("Up") && m_currentDir.has_parent_path()) 
        { 
            m_currentDir = m_currentDir.parent_path(); 
            refresh(); 
        }
		// Refresh button to manually refresh the directory listing.
        ImGui::SameLine(); if (ImGui::Button("Refresh")) refresh();

		// Display any errors that occur during directory reading or refreshing, as well as the count of skipped entries due to permissions or other issues. Finally, show the count of items being displayed.
        if (!refreshError.empty()) { ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "%s", refreshError.c_str()); }
        if (skippedCount > 0) { ImGui::TextColored(ImVec4(1,0.7f,0,1), "Skipped %d entries (permissions/special chars)", skippedCount); }
        ImGui::Text("Showing %d items", (int)entries.size());

		// Child region to display the list of files and directories. Each entry is selectable, and double-clicking a directory will navigate into it, while double-clicking a file will select it for loading.
        ImGui::BeginChild("file_list", ImVec2(600,300), true);
        for (auto &ent : entries) 
        {
            std::string name; try { name = ent.path().filename().string(); } catch(...) { name = "<unreadable>"; }
            bool is_dir = false; std::error_code ec; try { is_dir = ent.is_directory(ec); } catch(...) { is_dir = false; }
            std::string label = is_dir ? (name + "/") : name;
            std::string entPathStr; try { entPathStr = ent.path().string(); } catch(...) { entPathStr.clear(); }
            bool selected = (!selectedPath.empty() && selectedPath == entPathStr);
            
            if (ImGui::Selectable(label.c_str(), selected)) {
                if (!entPathStr.empty()) selectedPath = entPathStr;
                if (is_dir) {
                    m_currentDir = ent.path();
                    refreshError.clear();
                    skippedCount = 0;
                    refresh();
                    selectedPath.clear();
                }
            }

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
                    if (musicEntity) 
                    {
                        LoadMusicFromPath(sel);
                    }
                }
                ImGui::CloseCurrentPopup(); m_showOpenDialog = false; break;
            }
        }
        ImGui::EndChild();
        ImGui::Separator();

		// OK and Cancel buttons: OK will load the selected file, Cancel will just close the popup.
        if (ImGui::Button("OK") && !selectedPath.empty()) 
        {
            std::string sel = selectedPath;
            if (m_musicEntity) { m_entityManager.KillEntity(m_musicEntity); m_entityManager.Update(0.0f); m_musicEntity = nullptr; }
            Entity* musicEntity = m_entityManager.addEntity(EntityType::Default);
            
			// add the componenent and music if we have an entity bla bla bla, same as the double-click handler (might be better to refactor this into a helper function)
            if (musicEntity) 
            {
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

MusicVisualizerScene::MusicVisualizerScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager) : Scene(engine, entityManager), m_window(win) {}

MusicVisualizerScene::~MusicVisualizerScene() = default;

void MusicVisualizerScene::Update(float deltaTime)
{
    // Minimal update: process explosions and audio-reactive spawns similar to TileMapEditorScene
    // Pause visual updates when music is paused so effects stop on pause
    bool musicPaused = false;
    if (m_musicEntity) 
    {
        if (auto musicCmp = m_musicEntity->GetComponent<CMusic>()) 
        {
            musicPaused = (musicCmp->state == CMusic::State::Paused);
        }
    }
    if (!musicPaused) 
    {
        UpdateExplosions();
    }

    // Get the current FPS
    m_fps = m_gameEngine.GetFPSCounter().GetFPS();

    // Ensure current dir is initialized so the in-app file browser has a starting folder
    if (m_currentDir.empty()) m_currentDir = std::filesystem::current_path();

	// Draw ImGui UI for music loading and playback controls. This is done in Update so it is rendered within the main Music Visualizer, which makes more fucking sense.
    if (GImGui && GImGui->WithinFrameScope) 
    {
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

    // Display music status (loaded file or errors)
    if (!m_musicStatus.empty()) ImGui::TextUnformatted(m_musicStatus.c_str());

    // simple input
    ProcessInput();
}

void MusicVisualizerScene::Render()
{
}

void MusicVisualizerScene::DoAction() { }

void MusicVisualizerScene::RenderDebugOverlay()
{
    // Draw grid only
    sf::View prevView = m_window.getView();
    sf::View view = m_window.getDefaultView();
    m_window.setView(view);
    DrawGrid();
    m_window.setView(prevView);
}

void MusicVisualizerScene::HandleEvent(const std::optional<sf::Event>& event)
{
}

void MusicVisualizerScene::OnEnter() {}
void MusicVisualizerScene::OnExit() {}

void MusicVisualizerScene::LoadResources() { m_isLoaded = true; }
void MusicVisualizerScene::UnloadResources() { }

void MusicVisualizerScene::InitializeGame(sf::Vector2u windowSize)
{
    const float tileSize = 32.0f;
    int cols = static_cast<int>(windowSize.x / static_cast<unsigned int>(tileSize)) + 2;
    int rows = static_cast<int>(windowSize.y / static_cast<unsigned int>(tileSize)) + 2;
    m_tileMap = TileMap(cols, rows, tileSize);
}

void MusicVisualizerScene::DrawGrid()
{
    if (m_tileMap.width <= 0 || m_tileMap.height <= 0) return;
    for (int y = 0; y < m_tileMap.height; ++y) 
    {
        for (int x = 0; x < m_tileMap.width; ++x) 
        {
            sf::RectangleShape outline(sf::Vector2f(m_tileMap.tileSize, m_tileMap.tileSize));
            outline.setPosition(sf::Vector2f(x * m_tileMap.tileSize, y * m_tileMap.tileSize));
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineColor(sf::Color(200,200,200,60));
            outline.setOutlineThickness(1.0f);
            m_window.draw(outline);
        }
    }
}

void MusicVisualizerScene::ProcessInput()
{
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) 
    {
        m_gameEngine.m_window.close();
    }
}

void MusicVisualizerScene::LoadMusicFromPath(const std::string& path)
{
    if (m_musicEntity) { m_entityManager.KillEntity(m_musicEntity); m_entityManager.Update(0.0f); m_musicEntity = nullptr; }
    Entity* musicEntity = m_entityManager.addEntity(EntityType::Default);
    if (!musicEntity) return;
    auto* musicComponent = musicEntity->AddComponent<CMusic>(path, 80.f, true, true);
    musicComponent->state = CMusic::State::Playing;
    musicComponent->loop = true;
    m_musicEntity = musicEntity;
    m_entityManager.ProcessPending();
    if (auto ms = m_entityManager.GetMusicSystem()) ms->Process();
    m_musicStatus = std::string("Loaded: ") + path;
}

void MusicVisualizerScene::ToggleTileAt(int tx, int ty, bool setSolid)
{
}

void MusicVisualizerScene::UpdateExplosions()
{
    m_explosionCount = 0;
    auto now = std::chrono::high_resolution_clock::now();
    for (auto& entity : m_entityManager.getEntities()) 
    {
        if (entity->GetType() == EntityType::Explosion) 
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - entity->m_creationTime);
            if (elapsed.count() > 2900) 
            {
                entity->Destroy();
            } 
            else 
            {
                ++m_explosionCount;
                float fadeProgress = static_cast<float>(elapsed.count()) / 2900.0f;
                int newAlpha = static_cast<int>(80 * (1.0f - fadeProgress));

                auto shape = entity->GetComponent<CShape>();
                
                if (shape) 
                {
                    if (auto* explosion = dynamic_cast<CExplosion*>(shape)) 
                    {
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
