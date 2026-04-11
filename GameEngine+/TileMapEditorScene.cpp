#include "TileMapEditorScene.h"
#include "GameEngine.h"
#include "EntityManager.h"
#include <iostream>
#include "FileDialog.h"
#include "Entity.h"
#include "CTileMap.h"
#include "CTexture.h"
#include "CMusic.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>
// Internal context pointer used to safely check whether ImGui NewFrame() has been called
#include <imgui/imgui_internal.h>
#include <filesystem>

// Constructor - initializes the tile map editor scene with references to the game engine, render window, and entity manager
TileMapEditorScene::TileMapEditorScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager): Scene(engine, entityManager), m_window(win) {}

// Destructor - defaulted since we don't have any special cleanup logic, but we could add it if needed in the future
TileMapEditorScene::~TileMapEditorScene() = default;

void TileMapEditorScene::Update(float deltaTime)
{
    // Update shared FPS counter is handled by the engine; query it here for display
    m_fps = m_gameEngine.GetFPSCounter().GetFPS();
    // Disabled ImGui update while debugging visuals
    // ImGui::SFML::Update(m_gameEngine.m_window, frameTime);

    // ImGui toolbar (minimal)
    // Ensure instance current dir is initialized once
    if (m_currentDir.empty()) m_currentDir = std::filesystem::current_path();
    if (GImGui && GImGui->WithinFrameScope) {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
        ImGui::SetNextWindowBgAlpha(0.35f);
        ImGui::Begin("TileMap Editor", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("File: %s", m_currentFilename.empty() ? "<unsaved>" : m_currentFilename.c_str());
        ImGui::Text("Map: %d x %d, tileSize: %.1f", m_tileMap.width, m_tileMap.height, m_tileMap.tileSize);
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::Text("FPS (smoothed): %.1f", m_fps);
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::Text("FPS (instant): %.1f", m_gameEngine.GetFPSCounter().GetInstantFPS());
        ImGui::Separator();

        // Keep track of selected file from navigator
        static std::string s_selected_file;
        auto try_load_path = [&](const std::string& fullpath) {
            if (fullpath.empty()) return;
            std::string err;
            auto maybeMap = TileMap::LoadFromJSON(fullpath, &err);
            if (maybeMap) {
                m_tileMap = *maybeMap;
                m_currentFilename = fullpath;
                m_dirty = false;
                std::cout << "Loaded tilemap: " << fullpath << std::endl;
                // If we have an existing CTileMap entity, update its component so TileSystem will reprocess
                if (m_tileMapEntity) {
                    auto cmp = m_tileMapEntity->GetComponent<CTileMap>();
                    if (cmp) {
                        cmp->map = m_tileMap; // copy whole TileMap including tileset metadata
                        cmp->m_dirty = true;
                        cmp->m_processed = false;
                        m_entityManager.SetHasPendingTileMaps(true);
                        m_entityManager.Update(0.0f); // force immediate processing so textures attach when atlas available
                    }
                }
            } else {
                std::cerr << "Failed to load tilemap: " << err << std::endl;
            }
        };

        if (ImGui::Button("Load")) {
            // If a file is selected in the navigator, load it immediately
            if (!s_selected_file.empty()) {
                try_load_path(s_selected_file);
            } else {
                // Prefill load buffer with current folder if empty and open popup
                if (m_loadFilenameBuffer[0] == '\0') {
                    std::string pre = m_currentDir.string();
                    pre += std::filesystem::path::preferred_separator;
                    ImStrncpy(m_loadFilenameBuffer, pre.c_str(), sizeof(m_loadFilenameBuffer));
                }
                m_showLoadDialog = true; ImGui::OpenPopup("Load Map");
            }
        }
        if (ImGui::BeginPopupModal("Load Map", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("Path", m_loadFilenameBuffer, sizeof(m_loadFilenameBuffer));
            if (ImGui::Button("OK")) {
                std::string pathStr(m_loadFilenameBuffer);
                // If path is relative, resolve against current dir
                std::filesystem::path p(pathStr);
                if (!p.is_absolute()) p = m_currentDir / p;
                std::string path = p.string();
                if (!path.empty()) try_load_path(path);
                ImGui::CloseCurrentPopup(); m_showLoadDialog = false;
            }
            ImGui::SameLine(); if (ImGui::Button("Cancel")) { ImGui::CloseCurrentPopup(); m_showLoadDialog = false; }
            ImGui::EndPopup();
        }

        if (ImGui::Button("Save")) {
            // Prefill save buffer with current folder + default filename if empty
            if (m_saveFilenameBuffer[0] == '\0') {
                std::string pre = m_currentDir.string();
                pre += std::filesystem::path::preferred_separator;
                pre += (m_currentFilename.empty() ? "map.json" : std::filesystem::path(m_currentFilename).filename().string());
                ImStrncpy(m_saveFilenameBuffer, pre.c_str(), sizeof(m_saveFilenameBuffer));
            }
            m_showSaveDialog = true; ImGui::OpenPopup("Save Map");
        }
        if (ImGui::BeginPopupModal("Save Map", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("Path", m_saveFilenameBuffer, sizeof(m_saveFilenameBuffer));
            if (ImGui::Button("OK")) {
                std::string pathStr(m_saveFilenameBuffer);
                std::filesystem::path p(pathStr);
                if (!p.is_absolute()) p = m_currentDir / p;
                std::string path = p.string();
                if (!path.empty()) {
                    std::string err; if (m_tileMap.SaveToJSON(path, &err)) { m_currentFilename = path; m_dirty = false; 
                        // Also ensure saved file includes tileset metadata from current map
                    } else { std::cerr << "Error saving tilemap: " << err << std::endl; }
                }
                ImGui::CloseCurrentPopup(); m_showSaveDialog = false;
            }
            ImGui::SameLine(); if (ImGui::Button("Cancel")) { ImGui::CloseCurrentPopup(); m_showSaveDialog = false; }
            ImGui::EndPopup();
        }

        ImGui::Separator();
        // File navigator for current directory
        static std::vector<std::filesystem::directory_entry> s_entries;
        auto refresh_entries = [&]() {
            s_entries.clear();
            try {
                for (const auto& e : std::filesystem::directory_iterator(m_currentDir)) {
                    bool is_dir = false;
                    try { is_dir = e.is_directory(); } catch(...) { is_dir = false; }
                    if (is_dir) {
                        s_entries.push_back(e);
                        continue;
                    }
                    // Only include JSON files
                    std::string ext;
                    try { ext = e.path().extension().string(); } catch(...) { ext.clear(); }
                    // normalize to lower-case
                    for (auto &c : ext) c = (char)tolower((unsigned char)c);
                    if (ext == ".json") s_entries.push_back(e);
                }
                // Sort directories first, then files, each group alphabetically
                std::sort(s_entries.begin(), s_entries.end(), [](auto const &a, auto const &b){
                    bool a_dir = false, b_dir = false;
                    try { a_dir = a.is_directory(); } catch(...) { a_dir = false; }
                    try { b_dir = b.is_directory(); } catch(...) { b_dir = false; }
                    if (a_dir != b_dir) return a_dir > b_dir; // directories first
                    return a.path().filename().string() < b.path().filename().string();
                });
            } catch (...) { s_entries.clear(); }
        };

        if (ImGui::Button("Up") && m_currentDir.has_parent_path()) {
            m_currentDir = m_currentDir.parent_path();
            refresh_entries();
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh")) { refresh_entries(); }
        ImGui::SameLine(); ImGui::Text("Current folder: %s", m_currentDir.string().c_str());

        // Populate entries on first display
        if (s_entries.empty()) refresh_entries();

        ImGui::BeginChild("files_list", ImVec2(0, 200), true);
        for (size_t i = 0; i < s_entries.size(); ++i) {
            const auto& entry = s_entries[i];
            std::string name = entry.path().filename().string();
            bool is_dir = false;
            try { is_dir = entry.is_directory(); } catch (...) { is_dir = false; }
            std::string label = is_dir ? (name + "/") : name;
            std::string fullpath = (m_currentDir / entry.path().filename()).string();
            bool selected = (!s_selected_file.empty() && s_selected_file == fullpath);
            if (ImGui::Selectable(label.c_str(), selected)) {
                if (is_dir) {
                    m_currentDir = entry.path();
                    s_selected_file.clear();
                    refresh_entries();
                } else {
                    // populate load buffer with full relative path and mark selection
                    ImStrncpy(m_loadFilenameBuffer, fullpath.c_str(), sizeof(m_loadFilenameBuffer));
                    s_selected_file = fullpath;
                    // Double-click to load immediately
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        try_load_path(s_selected_file);
                    }
                }
            }
        }
        ImGui::EndChild();

        // Atlas loader UI: try to load adventure.png into engine atlas
        static std::string s_atlas_status;
        bool atlasLoaded = m_gameEngine.GetTextureManager().GetAtlas("adventure").has_value();
        ImGui::Separator();
        ImGui::Text("Atlas 'adventure': %s", atlasLoaded ? "Loaded" : "Not loaded");
        if (ImGui::Button("Load adventure.png into atlas")) {
            std::vector<std::filesystem::path> candidates = {
                m_currentDir / "adventure.png",
                m_currentDir / "assets" / "adventure.png",
                std::filesystem::current_path() / "assets" / "adventure.png",
                std::filesystem::current_path() / "adventure.png"
            };
            std::string found;
            std::error_code ec;
            for (auto &c : candidates) {
                if (!c.empty() && std::filesystem::exists(c, ec) && !ec) { found = c.string(); break; }
            }
                if (found.empty()) {
                s_atlas_status = "adventure.png not found in candidates";
                std::cerr << s_atlas_status << std::endl;
            } else {
                bool ok = m_gameEngine.GetTextureManager().LoadAtlas("adventure", found, 32, 32);
                s_atlas_status = ok ? (std::string("Loaded: ") + found) : (std::string("Failed to load: ") + found);
                if (ok) std::cout << "Texture atlas loaded: " << found << std::endl; else std::cerr << s_atlas_status << std::endl;
                    if (ok) {
                        // Ensure current tilemap knows which tileset to use so TileSystem will attach textures
                        m_tileMap.tilesetKey = "adventure";
                        m_tileMap.tilesetTileW = 32;
                        m_tileMap.tilesetTileH = 32;
                        // Mark CTileMap component dirty so TileSystem recreates tile entities with textures
                        if (m_tileMapEntity) {
                            auto cmp = m_tileMapEntity->GetComponent<CTileMap>();
                            if (cmp) {
                                cmp->map.tilesetKey = m_tileMap.tilesetKey;
                                cmp->map.tilesetTileW = m_tileMap.tilesetTileW;
                                cmp->map.tilesetTileH = m_tileMap.tilesetTileH;
                                cmp->m_dirty = true;
                                cmp->m_processed = false;
                                m_entityManager.SetHasPendingTileMaps(true);
                                m_entityManager.Update(0.0f); // force immediate processing
                            }
                        }
                    }
            }
        }
        if (!s_atlas_status.empty()) ImGui::TextUnformatted(s_atlas_status.c_str());
        // Tile entities textures panel
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Tile Entities (Textures)")) {
            auto& tileEntities = m_entityManager.getEntities(EntityType::Tile);
            ImGui::BeginChild("tile_textures", ImVec2(0, 200), true);
            int count = 0;
            for (Entity* te : tileEntities) {
                if (!te) continue;
                auto texComp = te->GetComponent<CTexture>();
                if (!texComp) continue;
                // Lookup atlas and rect
                auto atlasOpt = m_gameEngine.GetTextureManager().GetAtlas(texComp->atlasKey);
                if (!atlasOpt.has_value()) continue;
                auto atlasPtr = *atlasOpt;
                if (!atlasPtr) continue;
                auto tileRectOpt = atlasPtr->GetRectForTile((size_t)texComp->tileIndex);
                auto texPtr = atlasPtr->GetTexture();
                if (!tileRectOpt.has_value() || !texPtr) continue;
                auto tr = *tileRectOpt;

                // Create sprite for thumbnail
                sf::Sprite sprite(*texPtr);
                sprite.setTextureRect(sf::IntRect(sf::Vector2i(tr.x, tr.y), sf::Vector2i(tr.w, tr.h)));

                // Thumbnail size (scaled to 32 px region preserving aspect by simple fit)
                float thumb = 32.0f;
                sf::Vector2f thumbSize(thumb, thumb);

                std::string id = "tile_tex_" + std::to_string(count);
                if (ImGui::ImageButton(id.c_str(), sprite, thumbSize)) {
                    // Set brush to this tile value (map tile values are 1-based)
                    m_brushTileValue = texComp->tileIndex + 1;
                }
                ImGui::SameLine();
                ImGui::Text("%s [%d]", texComp->atlasKey.c_str(), texComp->tileIndex);
                ImGui::NewLine();
                ++count;
            }
            ImGui::EndChild();
            // Debug: show how many textured tile entities were found
            int texturedCount = 0;
            for (Entity* te : tileEntities) if (te && te->GetComponent<CTexture>()) ++texturedCount;
            ImGui::Text("Textured tile entities: %d", texturedCount);
        }

        // Atlas browser (debug): show all tiles from loaded 'adventure' atlas so user can pick one even if no tile entities exist
        ImGui::Separator();
        ImGui::Text("Atlas Browser (adventure)");
        auto atlasOpt = m_gameEngine.GetTextureManager().GetAtlas("adventure");
        if (!atlasOpt.has_value() || !(*atlasOpt)) {
            ImGui::TextUnformatted("(atlas not loaded)");
        } else {
            auto atlasPtr = *atlasOpt;
            auto texPtr = atlasPtr->GetTexture();
            if (!texPtr) {
                ImGui::TextUnformatted("(atlas texture missing)");
            } else {
                int tileCount = static_cast<int>(atlasPtr->TileCount());
                ImGui::Text("Tiles in atlas: %d", tileCount);
                const int cols = 8;
                float thumb = 32.0f;
                ImGui::BeginChild("atlas_grid", ImVec2(0, 200), true);
                for (int i = 0; i < tileCount; ++i) {
                    auto trOpt = atlasPtr->GetRectForTile((size_t)i);
                    if (!trOpt.has_value()) continue;
                    auto tr = *trOpt;
                    sf::Sprite sprite(*texPtr);
                    sprite.setTextureRect(sf::IntRect(sf::Vector2i(tr.x, tr.y), sf::Vector2i(tr.w, tr.h)));
                    std::string id = std::string("atlas_tile_") + std::to_string(i);
                    if (ImGui::ImageButton(id.c_str(), sprite, sf::Vector2f(thumb, thumb))) {
                        m_brushTileValue = i + 1; // atlas 0-based -> map 1-based
                    }
                    if ((i % cols) != (cols - 1)) ImGui::SameLine();
                }
                ImGui::EndChild();
            }
        }


        // Music loader UI.. I can use imgui to test loading of and playing of music
        ImGui::Separator();
        ImGui::Text("Music (assets)");
        static std::string s_music_status;

		// Try to load ambition.mp3 from candidates and play it using CMusic component on a temporary entity
        if (ImGui::Button("Load assets/ambition.mp3")) 
        {
            std::vector<std::filesystem::path> candidates = { m_currentDir / "ambition.mp3", m_currentDir / "assets" / "ambition.mp3", 
                                                                            std::filesystem::current_path() / "assets" / "ambition.mp3",
                                                                            std::filesystem::current_path() / "ambition.mp3"
            };

			// Check candidates for existence and pick the first one found
            std::string found;
			std::error_code errorCode; // To avoid exceptions in the filesystem operations. If an error occurs, the error code will be set and we can check it instead of catching exceptions.
            for (auto &candidate : candidates) 
            {
                if (!candidate.empty() && std::filesystem::exists(candidate, errorCode) && !errorCode) 
                { 
                    found = candidate.string(); 
                    break; 
                }
            }
			// If found, create a music entity with CMusic component to play it. Otherwise, log an error.
			// Note: the music file must be a valid format supported by SFML (e.g. OGG, WAV, FLAC) and not too large to load into memory, since SFML's sf::Music streams from file but still needs 
            // to load the file header and manage it with sf::Music instance. If the file is very large or in an unsupported format, loading may fail. ....Shit we can load Flac????? HollllyyyySsseeehhhhiiiit
            if (found.empty()) // check if there is nothing, report error
            {
                s_music_status = "ambition.mp3 not found in candidates";
                std::cerr << s_music_status << std::endl;
            } 
            // otherwise create 
            else {
                // Create an entity and attach a music component. Constructor: (path, volume, looped, playOnStart) and play....
                Entity* musicEntity = m_entityManager.addEntity(EntityType::Default);
                if (musicEntity) 
                {
                    auto* musicComponent = musicEntity->AddComponent<CMusic>(found, 80.f, true, true);
                    musicComponent->state = CMusic::State::Playing;
                    musicComponent->loop = true;
                    // Force immediate processing so MusicSystem opens/plays the file now
                    m_entityManager.Update(0.0f);
                    s_music_status = std::string("Loaded: ") + found;
                    std::cout << s_music_status << std::endl;
                }
				// If entity or component creation failed, log an error
                else 
                {
                    s_music_status = "Failed to create music entity";
                    std::cerr << s_music_status << std::endl;
                }
            }
        }

		// Display music status (loaded file or errors)
        if (!s_music_status.empty()) ImGui::TextUnformatted(s_music_status.c_str());

        ImGui::End();
    }

    // simple input handling
    ProcessInput();

    // ImGui draw will be rendered centrally by the engine after scene rendering
}

void TileMapEditorScene::Render()
{
    // Scene render: nothing (grid and overlays are drawn after ImGui by RenderDebugOverlay)
}

void TileMapEditorScene::RenderDebugOverlay()
{
    // Draw debug overlay using a view that maps world (0,0..window) to screen so tile coordinates align
    sf::View prevView = m_window.getView();
    // Use the default view to draw overlay aligned with window pixels
    sf::View view = m_window.getDefaultView();
    m_window.setView(view);

    // Draw grid and highlight on top of entities (no fullscreen diagnostic overlay)
    DrawGrid();
    if (m_lastClickedX >= 0 && m_lastClickedY >= 0) {
        sf::RectangleShape highlight(sf::Vector2f(m_tileMap.tileSize, m_tileMap.tileSize));
        highlight.setPosition(sf::Vector2f(m_lastClickedX * m_tileMap.tileSize, m_lastClickedY * m_tileMap.tileSize));
        highlight.setFillColor(sf::Color::Transparent);
        highlight.setOutlineColor(sf::Color::Yellow);
        highlight.setOutlineThickness(2.0f);
        m_window.draw(highlight);
    }

    // Normal rendering: tile entities are drawn by the engine's TileSystem; do not duplicate here

    m_window.setView(prevView);
}

void TileMapEditorScene::DoAction()
{
}

void TileMapEditorScene::HandleEvent(const std::optional<sf::Event>& event)
{
    // update mouse world pos
    sf::Vector2i mp = sf::Mouse::getPosition(m_window);
    sf::Vector2f mw = m_window.mapPixelToCoords(mp);
    m_mouseWorld = Vec2(mw.x, mw.y);

    // Forward events from engine (mouse clicks should toggle tiles unless mouse is over an ImGui window)
    if (event.has_value()) {
        // Only block scene input when the mouse is over any ImGui window (toolbar, dialogs)
        bool uiHover = false;
        if (GImGui && GImGui->WithinFrameScope) {
            uiHover = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsAnyItemHovered();
        }
        if (!uiHover) {
            // edge-detect mouse presses to toggle tiles once per click
            bool leftDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
            bool rightDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);
            if (leftDown && !m_prevLeftMouse) {
                int tx = static_cast<int>(std::floor(m_mouseWorld.x / m_tileMap.tileSize));
                int ty = static_cast<int>(std::floor(m_mouseWorld.y / m_tileMap.tileSize));
                ToggleTileAt(tx, ty, true);
                m_lastClickedX = tx; m_lastClickedY = ty;
                m_toggleLog.push_back("L: (" + std::to_string(tx) + "," + std::to_string(ty) + ")");
                // update preview immediately
                ImGui::SetWindowFocus("TileMap Editor");
            }
            if (rightDown && !m_prevRightMouse) {
                int tx = static_cast<int>(std::floor(m_mouseWorld.x / m_tileMap.tileSize));
                int ty = static_cast<int>(std::floor(m_mouseWorld.y / m_tileMap.tileSize));
                ToggleTileAt(tx, ty, false);
                m_lastClickedX = tx; m_lastClickedY = ty;
                m_toggleLog.push_back("R: (" + std::to_string(tx) + "," + std::to_string(ty) + ")");
                ImGui::SetWindowFocus("TileMap Editor");
            }
            m_prevLeftMouse = leftDown;
            m_prevRightMouse = rightDown;
        }
        // keyboard handled in ProcessInput()
    }
}

void TileMapEditorScene::OnEnter(){}
void TileMapEditorScene::OnExit(){}
void TileMapEditorScene::LoadResources() { m_isLoaded = true; }
void TileMapEditorScene::UnloadResources() { }

void TileMapEditorScene::InitializeGame(sf::Vector2u windowSize)
{
    // create a default map sized to the window
    const float tileSize = 32.0f;
    int cols = static_cast<int>(windowSize.x / static_cast<unsigned int>(tileSize)) + 2;
    int rows = static_cast<int>(windowSize.y / static_cast<unsigned int>(tileSize)) + 2;
    m_tileMap = TileMap(cols, rows, tileSize);

    // Create a CTileMap entity so the engine's TileSystem can render map as entities
    if (m_entityManager.getEntities().size() >= 0) {
        if (m_tileMapEntity) {
            m_entityManager.KillEntity(m_tileMapEntity);
            m_tileMapEntity = nullptr;
        }
        m_tileMapEntity = m_entityManager.CreateTileMapEntity(m_tileMap);
    }

    // ensure engine texture manager has default atlas preloaded in GameEngine ctor
}

void TileMapEditorScene::DrawGrid()
{
    if (m_tileMap.width <= 0 || m_tileMap.height <= 0) return;
    // Draw using the current view (world coordinates) so grid aligns with mouse/world coords
    for (int y = 0; y < m_tileMap.height; ++y) {
        for (int x = 0; x < m_tileMap.width; ++x) {
            // Only draw a filled grey rectangle for tiles when no texture atlas is configured.
            // When a tileset/atlas is present we rely on the RenderSystem to draw textured sprites
            // and avoid drawing the grey fill which would blend over the texture thumbnail.
            if (m_tileMap.IsSolid(x,y) && m_tileMap.tilesetKey.empty()) {
                sf::RectangleShape rect(sf::Vector2f(m_tileMap.tileSize, m_tileMap.tileSize));
                rect.setPosition(sf::Vector2f(x * m_tileMap.tileSize, y * m_tileMap.tileSize));
                rect.setFillColor(sf::Color(160,160,160,220));
                rect.setOutlineColor(sf::Color::Transparent);
                m_window.draw(rect);
            }
            // grid lines
            sf::RectangleShape outline(sf::Vector2f(m_tileMap.tileSize, m_tileMap.tileSize));
            outline.setPosition(sf::Vector2f(x * m_tileMap.tileSize, y * m_tileMap.tileSize));
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineColor(sf::Color(200,200,200,60));
            outline.setOutlineThickness(1.0f);
            m_window.draw(outline);
        }
    }
}

void TileMapEditorScene::ProcessInput()
{
    // handle keyboard shortcuts using real-time state (need to use in Update loop so keypress is captured)
    bool ctrlS = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
    if (ctrlS && !m_prevCtrlS) {
        std::string err;
        const std::string filename = "assets/editor_map.json";
        if (!m_tileMap.SaveToJSON(filename, &err)) {
            std::cerr << "Error saving tilemap: " << err << std::endl;
        } else {
            std::cout << "Tilemap saved to " << filename << std::endl;
        }
    }
    m_prevCtrlS = ctrlS;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
        m_gameEngine.m_window.close();
    }

    // show save dialog on Ctrl+Shift+S (for interactive save-as)
    bool ctrlShiftS = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
    static bool prevCtrlShiftS = false;
    if (ctrlShiftS && !prevCtrlShiftS) {
        auto maybePath = ShowSaveFileDialog("JSON files\0*.json\0All files\0*.*\0", "map.json", "assets"); 
        if (maybePath) {
            std::string err;
            if (!m_tileMap.SaveToJSON(*maybePath, &err)) {
                std::cerr << "Error saving tilemap: " << err << std::endl;
            } else {
                std::cout << "Tilemap saved to " << *maybePath << std::endl;
            }
        }
    }
    prevCtrlShiftS = ctrlShiftS;
}

void TileMapEditorScene::ToggleTileAt(int tx, int ty, bool setSolid)
{
    if (!m_tileMap.InBounds(tx,ty)) return;
    m_tileMap.SetTile(tx, ty, setSolid ? m_brushTileValue : 0);
    m_dirty = true;
    std::cout << "ToggleTileAt: (" << tx << "," << ty << ") -> " << (setSolid ? m_brushTileValue : 0) << std::endl;

    // Update attached CTileMap component if we created one
    if (m_tileMapEntity) {
        auto cmp = m_tileMapEntity->GetComponent<CTileMap>();
        if (cmp) {
            cmp->SetTile(tx, ty, setSolid ? m_brushTileValue : 0);
            cmp->m_dirty = true;
            cmp->m_processed = false;
            m_entityManager.SetHasPendingTileMaps(true);
            // Process tile system immediately so tile entities appear this frame
            m_entityManager.Update(0.0f);
            std::cout << "Tile entities now: " << m_entityManager.getEntities(EntityType::Tile).size() << std::endl;
        }
    }
}
