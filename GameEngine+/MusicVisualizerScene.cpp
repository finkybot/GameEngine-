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

MusicVisualizerScene::MusicVisualizerScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager)
    : Scene(engine, entityManager), m_window(win)
{
}

MusicVisualizerScene::~MusicVisualizerScene() = default;

void MusicVisualizerScene::Update(float deltaTime)
{
    // Minimal update: process explosions and audio-reactive spawns similar to TileMapEditorScene
    // Pause visual updates when music is paused so effects stop on pause
    bool musicPaused = false;
    if (m_musicEntity) {
        if (auto cm = m_musicEntity->GetComponent<CMusic>()) {
            musicPaused = (cm->state == CMusic::State::Paused);
        }
    }
    if (!musicPaused) {
        UpdateExplosions();
    }

    // Get the current FPS
    m_fps = m_gameEngine.GetFPSCounter().GetFPS();

    // Ensure current dir is initialized so the in-app file browser has a starting folder
    if (m_currentDir.empty()) m_currentDir = std::filesystem::current_path();


    if (GImGui && GImGui->WithinFrameScope) {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
        ImGui::Begin("Music Visualizer", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Music Visualizer - FPS: %.1f", m_fps);

        // Music loader UI.. keep inside the Music Visualizer window so it is not placed in the default debug window
        ImGui::Separator();
        ImGui::Text("Music (assets)");
        static std::string s_music_status;

        // In-app file browser popup to avoid native dialog z-order issues
        ImGui::SameLine();
        if (ImGui::Button("Browse...")) {
            m_showOpenDialog = true; ImGui::OpenPopup("Open Audio File");
        }
        if (m_showOpenDialog && ImGui::BeginPopupModal("Open Audio File", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            // Simple file browser: show current directory and list .mp3/.ogg/.wav/.flac files
            static std::vector<std::filesystem::directory_entry> entries;
            static std::string selectedPath;
            static std::string refreshError;
            static int skippedCount = 0;
            static bool showNonAudio = false;
            auto refresh = [&]() {
                entries.clear();
                refreshError.clear();
                skippedCount = 0;
                try {
                    // Use skip_permission_denied to avoid exceptions on unreadable entries
                    std::filesystem::directory_options opts = std::filesystem::directory_options::skip_permission_denied;
                    for (std::filesystem::directory_iterator it(m_currentDir, opts); it != std::filesystem::directory_iterator(); ++it) {
                        try {
                            std::error_code ec_entry;
                            const auto& e = *it;
                            bool is_dir = false;
                            try { is_dir = std::filesystem::is_directory(e.path(), ec_entry); } catch(...) { is_dir = false; }
                            if (ec_entry) { ++skippedCount; continue; } // skip problematic entries
                            if (is_dir) { entries.push_back(e); continue; }

                            // Only include common audio file extensions (case-insensitive)
                            if (showNonAudio) {
                                entries.push_back(e);
                            } else {
                                // Use u8string for better Unicode support
                                std::string ext;
                                try {
                                    auto u8ext = e.path().extension().u8string();
                                    ext = std::string(u8ext.begin(), u8ext.end());
                                } catch(...) { ext.clear(); }
                                for (auto &c : ext) c = (char)tolower((unsigned char)c);
                                if (ext == ".mp3" || ext == ".ogg" || ext == ".wav" || ext == ".flac") entries.push_back(e);
                            }
                        } catch (...) {
                            ++skippedCount; // entry caused an exception, skip it
                        }
                    }

                    std::sort(entries.begin(), entries.end(), [](auto const &a, auto const &b){
                        bool a_dir = false, b_dir = false;
                        std::error_code ea, eb;
                        try { a_dir = std::filesystem::is_directory(a.path(), ea); } catch(...) { a_dir = false; }
                        try { b_dir = std::filesystem::is_directory(b.path(), eb); } catch(...) { b_dir = false; }
                        if (a_dir != b_dir) return a_dir > b_dir;
                        // Use native path comparison for better Unicode handling
                        return a.path().filename() < b.path().filename();
                    });
                } catch (const std::exception& ex) {
                    entries.clear();
                    refreshError = std::string("Error: ") + ex.what();
                } catch (...) {
                    entries.clear();
                    refreshError = "Unknown error reading directory";
                }
            };

            if (entries.empty()) refresh();

            // Drive selector (Windows): allow quick change of drive root
            {
                std::vector<std::string> drives;
                for (char d = 'A'; d <= 'Z'; ++d) {
                    std::string root;
                    root.push_back(d);
                    root += ":\\";
                    std::error_code ec;
                    if (std::filesystem::exists(root, ec)) drives.push_back(root);
                }
                static int selDrive = -1;
                if (!drives.empty()) {
                    // ensure selDrive is valid
                    if (selDrive < 0 || selDrive >= (int)drives.size()) selDrive = 0;
                    // build items string for combo
                    std::string items;
                    for (size_t i = 0; i < drives.size(); ++i) {
                        items += drives[i];
                        items.push_back('\0');
                    }
                    if (ImGui::Combo("Drive", &selDrive, items.c_str())) {
                        m_currentDir = drives[selDrive];
                        refresh();
                    }
                }
            }

            // Optionally show non-audio files for debugging / browsing
            ImGui::SameLine(); ImGui::Checkbox("Show non-audio files", &showNonAudio);

            ImGui::Text("Current folder: %s", m_currentDir.string().c_str());
            if (ImGui::Button("Up") && m_currentDir.has_parent_path()) { m_currentDir = m_currentDir.parent_path(); refresh(); }
            ImGui::SameLine(); if (ImGui::Button("Refresh")) refresh();

            // Show error/status info
            if (!refreshError.empty()) { ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "%s", refreshError.c_str()); }
            if (skippedCount > 0) { ImGui::TextColored(ImVec4(1,0.7f,0,1), "Skipped %d entries (permissions/special chars)", skippedCount); }
            ImGui::Text("Showing %d items", (int)entries.size());

            ImGui::BeginChild("file_list", ImVec2(600,300), true);
            for (auto &ent : entries) 
            {
                // Use u8string for better Unicode filename support
                std::string name;
                try {
                    auto u8name = ent.path().filename().u8string();
                    name = std::string(u8name.begin(), u8name.end());
                } catch (...) {
                    name = "<unreadable>";
                }
                bool is_dir = false; std::error_code ec; try { is_dir = ent.is_directory(ec); } catch(...) { is_dir = false; }
                std::string label = is_dir ? (name + "/") : name;
                std::string entPathStr;
                try { entPathStr = ent.path().string(); } catch (...) { entPathStr.clear(); }
                bool selected = (!selectedPath.empty() && selectedPath == entPathStr);
                if (ImGui::Selectable(label.c_str(), selected)) {
                    if (!entPathStr.empty()) selectedPath = entPathStr;
                    if (is_dir) { m_currentDir = ent.path(); refreshError.clear(); skippedCount = 0; refresh(); selectedPath.clear(); }
                    else {
                        // single click selects file; double-click will accept
                        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                            // load immediately
                            std::string sel = selectedPath;
                            if (!sel.empty()) {
                                if (m_musicEntity) { m_entityManager.KillEntity(m_musicEntity); m_entityManager.Update(0.0f); m_musicEntity = nullptr; }
                                Entity* musicEntity = m_entityManager.addEntity(EntityType::Default);
                                if (musicEntity) {
                                    auto* musicComponent = musicEntity->AddComponent<CMusic>(sel, 80.f, true, true);
                                    musicComponent->state = CMusic::State::Playing; musicComponent->loop = true; m_musicEntity = musicEntity;
                                    m_entityManager.ProcessPending(); if (auto ms = m_entityManager.GetMusicSystem()) ms->Process();
                                    s_music_status = std::string("Loaded: ") + sel; std::cout << s_music_status << std::endl;
                                }
                            }
                            ImGui::CloseCurrentPopup(); m_showOpenDialog = false; break;
                        }
                    }
                }
            }
            ImGui::EndChild();

            ImGui::Separator();
            if (ImGui::Button("OK") && !selectedPath.empty()) {
                std::string sel = selectedPath;
                if (m_musicEntity) { m_entityManager.KillEntity(m_musicEntity); m_entityManager.Update(0.0f); m_musicEntity = nullptr; }
                Entity* musicEntity = m_entityManager.addEntity(EntityType::Default);
                if (musicEntity) {
                    auto* musicComponent = musicEntity->AddComponent<CMusic>(sel, 80.f, true, true);
                    musicComponent->state = CMusic::State::Playing; musicComponent->loop = true; m_musicEntity = musicEntity;
                    m_entityManager.ProcessPending(); if (auto ms = m_entityManager.GetMusicSystem()) ms->Process();
                    s_music_status = std::string("Loaded: ") + sel; std::cout << s_music_status << std::endl;
                }
                ImGui::CloseCurrentPopup(); m_showOpenDialog = false;
            }
            ImGui::SameLine(); if (ImGui::Button("Cancel")) { ImGui::CloseCurrentPopup(); m_showOpenDialog = false; }
            ImGui::EndPopup();
        }

        // ---- Audio-reactive spawner window (bottom-right) ----
        if (GImGui && GImGui->WithinFrameScope) {
            // Small floating window anchored bottom-right
            ImVec2 winSize(300, 120);
            ImVec2 pos((float)m_window.getSize().x - winSize.x - 10.0f, (float)m_window.getSize().y - winSize.y - 10.0f);
            ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);
            ImGui::Begin("Audio Reactive", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
            // Toggle reactive spawning
            ImGui::Checkbox("Enable Reactive Spawn", &m_audioReactive);
            ImGui::SliderFloat("Threshold", &m_spawnThreshold, 0.0001f, 0.1f, "%.4f");
            ImGui::SliderFloat("Cooldown (s)", &m_spawnCooldown, 0.02f, 1.0f, "%.2f");

            // show level for the current music entity (if any)
            float level = 0.0f;
            bool hasBuffer = false;
            bool musicPlaying = true;

            // We query the music entity level from MusicSystem and check if the analysis buffer is available to indicate whether level is actively being analyzed.
            // Determine whether the music entity is playing first, then query MusicSystem for the current level.
            if (m_musicEntity)
            {
				// Check if music is playing before querying level to avoid showing reactive spawns when paused
                if (auto cm = m_musicEntity->GetComponent<CMusic>()) 
                {
                    musicPlaying = (cm->state == CMusic::State::Playing);
                }

				// Query MusicSystem for current level and whether analysis buffer is available this will allow us to indicate whether analysis is active.
                if (auto musicSystem = m_entityManager.GetMusicSystem()) 
                {
                    // Ensure MusicSystem has processed this frame so level is up-to-date for reactive spawning
                    musicSystem->Process();
                    level = musicSystem->GetLevel(m_musicEntity->GetId());
                    hasBuffer = musicSystem->HasAnalysisBuffer(m_musicEntity->GetId());
                }
            }
            ImGui::Text("Level: %.4f", level);
            ImGui::SameLine(); ImGui::TextUnformatted(hasBuffer ? "(analyzing)" : "(no analysis)");

            // advance spawn timer
            m_spawnTimer += ImGui::GetIO().DeltaTime;

            // If enabled, music must be playing and level above threshold and cooldown passed spawn shape entity
            if (m_audioReactive && musicPlaying && level > m_spawnThreshold) 
            {
                if (m_spawnTimer >= m_spawnCooldown) {
                    // spawn a circle near bottom-right area
                    Entity* e = m_entityManager.addEntity(EntityType::Explosion);
                    if (e) 
                    {
                        // random size
                        float size = 6.0f + static_cast<float>(std::rand() % 36); // 6..41
                        // Use AddComponentPtr<CShape> so RenderSystem can find it via GetComponent<CShape>()
                        auto circle = std::make_unique<CExplosion>(size);
                        // random color
                        int r = 100 + (std::rand() % 156);
                        int g = 100 + (std::rand() % 156);
                        int b = 100 + (std::rand() % 156);
                        circle->SetColor((float)r, (float)g, (float)b, 220);
                        e->AddComponentPtr<CShape>(std::move(circle));

                        // place near center of the screen with small random jitter so they are visible
                        float cx = static_cast<float>(m_window.getSize().x) * 0.5f;
                        float cy = static_cast<float>(m_window.getSize().y) * 0.5f;
                       
                        // expand spawn area around center so shapes spread over a larger central region
                        float jitter = 250.0f; // pixels - +-jitter around center
                        float x = cx + (static_cast<float>(std::rand() % (static_cast<int>(jitter * 2 + 1))) - jitter);
                        float y = cy + (static_cast<float>(std::rand() % (static_cast<int>(jitter * 2 + 1))) - jitter);
                        
                        // Ensure the entity has a transform so RenderSystem positions it
                        // Add a CTransform with initial position/velocity instead of relying on it existing
                        auto* t = e->AddComponent<CTransform>(Vec2(x, y), Vec2(0.0f, -40.0f - static_cast<float>(std::rand() % 120)));
                        (void)t; // silence unused variable warnings

                        // Commit the spawned entity immediately so it will be visible this frame
                        m_entityManager.ProcessPending();

                        // reset timer and let EntityManager add entity on next update
                        // log spawn to console for debugging
                        std::cout << "AudioReactive: Spawned entity id=" << e->GetId() << " size=" << size << " pos=(" << x << "," << y << ")" << std::endl;
                        m_spawnTimer = 0.0f;
                    }
                }
            }

            ImGui::End();
        }

    // Display music status (loaded file or errors)
    if (!s_music_status.empty()) ImGui::TextUnformatted(s_music_status.c_str());

        // Playback controls for the loaded music entity
        if (m_musicEntity) 
        {
			// Show controls only if the entity has a CMusic component (it should, but we check to be safe)
            if (auto cm = m_musicEntity->GetComponent<CMusic>()) 
            {
                // State indicator
                const char* stateText = "Unknown";
                
				// Convert the CMusic::State enum to a human-readable string for display
                switch (cm->state) 
                {
                    case CMusic::State::Playing: stateText = "Playing"; break;
                    case CMusic::State::Paused: stateText = "Paused"; break;
                    case CMusic::State::Stopped: stateText = "Stopped"; break;
                }
				ImGui::Text("State: %s", stateText); // and display current state

                // Play / Pause / Stop buttons
                if (ImGui::Button("Play")) 
                {
                    cm->state = CMusic::State::Playing;
                    if (auto ms = m_entityManager.GetMusicSystem()) ms->Process();
                }
                ImGui::SameLine();
                if (ImGui::Button("Pause")) 
                {
                    cm->state = CMusic::State::Paused;
                    if (auto ms = m_entityManager.GetMusicSystem()) ms->Process();
                }
                ImGui::SameLine();
                if (ImGui::Button("Stop")) 
                {
                    cm->state = CMusic::State::Stopped;
                    if (auto ms = m_entityManager.GetMusicSystem()) ms->Process();
                }

                // Volume slider
                float vol = cm->volume;
                if (ImGui::SliderFloat("Volume", &vol, 0.0f, 100.0f, "%.0f")) 
                {
                    cm->volume = vol;
                    if (auto ms = m_entityManager.GetMusicSystem()) ms->Process();
                }
            }
        }

        ImGui::End();
    }

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
