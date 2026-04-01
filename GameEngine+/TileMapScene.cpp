#include "TileMapScene.h"
#include "GameEngine.h"
#include "EntityManager.h"
#include "CTileMap.h"
#include <iostream>
#include "Raycast.h"
#include <SFML/Graphics/RectangleShape.hpp>
#include <Utils.h>
#include "Vec2.h"
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Event.hpp>
#include "Entity.h"


// Constructor - initializes the tile map scene with a reference to the game engine and the render window, and sets up the entity manager
TileMapScene::TileMapScene(GameEngine& engine, sf::RenderWindow& win) : Scene(engine), m_window(win)
{
    EntityManager* em = new EntityManager(win);
    m_entityManager = em;
}


// Destructor - defaulted since we don't have any special cleanup logic, but we could add it if needed in the future
TileMapScene::~TileMapScene() = default;


// Process debug toggle key (D) to show/hide visual debug overlays
void TileMapScene::ProcessDebugToggle()
{
    bool debugToggle = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
    if (debugToggle && !m_prevDebugKeyDown) m_visualDebug = !m_visualDebug;
    m_prevDebugKeyDown = debugToggle;
}


// Helper to create a RaycastHit when ray starts inside a solid tile, since DDA will return no hit in this case
void TileMapScene::ProcessMouseDrag(bool mouseDown, const Vec2& mouseWorld)
{
    // Handle mouse drag state transitions and perform raycast on release
	if (mouseDown && !m_prevMouseDown) // if mouse is down and was not down in previous frame, start dragging
    {
		m_dragging = true;          // toggle dragging
		m_dragStart = mouseWorld;   // record drag start position
		m_previewActive = true;     // enable preview line while dragging
    }
	else if (!mouseDown && m_prevMouseDown) // if mouse is up and was down in previous frame, end dragging and perform raycast
    {
		if (m_dragging) // only perform raycast if we were dragging, otherwise it was just a click without movement
        {
            m_dragging = false;                 // end dragging
            m_dragEnd = mouseWorld;             // record drag end position    
			Vec2 dir = m_dragEnd - m_dragStart; // calculate drag direction vector
			float dragLen = dir.Mag();          // calculate drag length (magnitude of direction vector)

			// If the drag length is very small, we can treat it as a click rather than a drag. We use this to toggle a tile solid/not solid state. I'm using a very small threshold of 0.001f but it can be adjusted based on testing and needs.
            if (dragLen <= 0.001f)              
            {
                // Treat as a click (no meaningful drag): toggle the tile at the start cell
                int tx = static_cast<int>(std::floor(m_dragStart.x / m_testMap.tileSize));
                int ty = static_cast<int>(std::floor(m_dragStart.y / m_testMap.tileSize));
				ToggleTileAt(tx, ty);
                m_previewActive = false;
                m_prevMouseDown = mouseDown;
                return;
            }
            dir = dir.GetUnitVec();

            m_debugLines.clear();
            m_debugPoints.clear();

            int startTileX = static_cast<int>(std::floor(m_dragStart.x / m_testMap.tileSize));
            int startTileY = static_cast<int>(std::floor(m_dragStart.y / m_testMap.tileSize));
            bool startSolid = m_testMap.InBounds(startTileX, startTileY) && m_testMap.IsSolid(startTileX, startTileY);

            m_visitedCells.clear();
            RaycastHit rh_ignore = RaycastTilemapDDA(m_dragStart, dir, m_testMap, dragLen, startSolid, &m_visitedCells);
            RaycastHit rh_force = MakeStartCellHit(startTileX, startTileY, m_dragStart);

            // Build sampled visited list for visualization
            m_visitedCells.clear();
            float step = std::max(1.0f, m_testMap.tileSize * 0.25f);
            for (float t = 0.0f; t <= dragLen; t += step)
            {
                Vec2 p(m_dragStart.x + dir.x * t, m_dragStart.y + dir.y * t);
                int cx = static_cast<int>(std::floor(p.x / m_testMap.tileSize));
                int cy = static_cast<int>(std::floor(p.y / m_testMap.tileSize));
                if (!m_testMap.InBounds(cx, cy)) continue;
                if (std::find(m_visitedCells.begin(), m_visitedCells.end(), std::pair<int,int>(cx,cy)) == m_visitedCells.end())
                    m_visitedCells.push_back({cx, cy});
            }

            RaycastHit rh;
            if (startSolid)
            {
                rh = rh_force;
                if (rh_ignore.hit && (rh_ignore.tileX != rh_force.tileX || rh_ignore.tileY != rh_force.tileY))
                {
                    m_debugLines.push_back({ m_dragStart, rh_ignore.position });
                    m_rawHitPoints.push_back(rh_ignore.position);
                }
            }
            else
            {
                rh = rh_ignore;
            }

            if (rh.hit)
            {
                Vec2 hitPos = rh.position;
                float proj = (hitPos.x - m_dragStart.x) * dir.x + (hitPos.y - m_dragStart.y) * dir.y;
                if (proj > dragLen)
                {
                    hitPos = Vec2(m_dragStart.x + dir.x * dragLen, m_dragStart.y + dir.y * dragLen);
                }
                m_debugLines.push_back({ m_dragStart, hitPos });
                m_debugPoints.push_back(hitPos);
                m_rawHitPoints.push_back(rh.position);
            }
            else
            {
                m_debugLines.push_back({ m_dragStart, m_dragEnd });
                m_debugPoints.push_back(m_dragEnd);
            }

            m_previewActive = false;
        }
    }
    else if (mouseDown && m_dragging)
    {
        m_dragEnd = mouseWorld;
        m_previewLine = { m_dragStart, m_dragEnd };
    }

    m_prevMouseDown = mouseDown;
}


// Process key input to close the window when any key is pressed
void TileMapScene::ProcessKeyInput(bool keyDown) { if (keyDown) m_gameEngine.m_window.close(); }


// Update method - handles events, updates the entity manager, and prepares debug visualization data for rendering
void TileMapScene::Update(float /*deltaTime*/)
{
    // Handle events (SFML 3.0: pollEvent returns std::optional<sf::Event>)
    while (auto eventOpt = m_gameEngine.m_window.pollEvent())
    {
        // ImGui::SFML::ProcessEvent(m_gameEngine.m_window, *eventOpt);

        if (eventOpt->is<sf::Event::Closed>())
        {
            m_gameEngine.m_window.close();
        }

        HandleEvent(eventOpt);
    }



    // simple update: run entity manager update
    m_entityManager->Update(0.016f);
}


// Render method - draws the tile grid and any debug visualization overlays
void TileMapScene::Render()
{
    // draw tile grid (solid tiles)
    DrawTileGrid();
    DrawDebugLines();
    DrawHitPoints();
    DrawRawHitPoints();
    DrawVisitedCells();
    DrawPreviewLine();
}


// DoAction method - currently empty, but could be used for game logic that needs to run on a fixed timestep or in response to certain conditions
void TileMapScene::DoAction()
{
}


// Draw the tile grid by iterating over the tile map and drawing a semi-transparent rectangle for each solid tile
void TileMapScene::DrawTileGrid()
{
    if (m_testMap.width <= 0 || m_testMap.height <= 0) return;
    for (int y = 0; y < m_testMap.height; ++y)
    {
        for (int x = 0; x < m_testMap.width; ++x)
        {
            if (m_testMap.IsSolid(x, y))
            {
                sf::RectangleShape rect(sf::Vector2f(m_testMap.tileSize, m_testMap.tileSize));
                rect.setPosition(sf::Vector2f(x * m_testMap.tileSize, y * m_testMap.tileSize));
                rect.setFillColor(sf::Color(100, 100, 100, 150));
                rect.setOutlineColor(sf::Color::Transparent);
                m_window.draw(rect);
            }
        }
    }
}


// Draw debug lines for raycasts, using red color for visibility. Only draws if visual debug mode is enabled.
void TileMapScene::DrawDebugLines()
{
    if (!m_visualDebug) return;
    for (const auto& pr : m_debugLines)
    {
        sf::Vertex line[] =
        {
            sf::Vertex(sf::Vector2f(pr.first.x, pr.first.y), sf::Color::Red),
            sf::Vertex(sf::Vector2f(pr.second.x, pr.second.y), sf::Color::Red)
        };
        m_window.draw(line, 2, sf::PrimitiveType::Lines);
    }
}


// Draw hit points as yellow circles, only if visual debug mode is enabled. This shows the final hit position after clamping to the ray length.
void TileMapScene::DrawHitPoints()
{
    if (!m_visualDebug) return;
    for (const auto& p : m_debugPoints)
    {
        sf::CircleShape dot(4.0f);
        dot.setFillColor(sf::Color::Yellow);
        dot.setOrigin(sf::Vector2f(4.0f, 4.0f));
        dot.setPosition(sf::Vector2f(p.x, p.y));
        m_window.draw(dot);
    }
}


// Draw raw hit points (before clamping) as blue circles, only if visual debug mode is enabled. This can show the actual intersection point with the tile boundary, which may be outside the ray length if the ray starts inside a solid tile.
void TileMapScene::DrawRawHitPoints()
{
    if (!m_visualDebug) return;
    for (const auto& p : m_rawHitPoints)
    {
        sf::CircleShape dot(3.0f);
        dot.setFillColor(sf::Color::Blue);
        dot.setOrigin(sf::Vector2f(3.0f, 3.0f));
        dot.setPosition(sf::Vector2f(p.x, p.y));
        m_window.draw(dot);
    }
}


// Draw visited cells as semi-transparent blue rectangles, only if visual debug mode is enabled. This shows which cells have been visited during raycasting or other pathfinding operations.
void TileMapScene::DrawVisitedCells()
{
    if (!m_visualDebug) return;
    for (const auto& cell : m_visitedCells)
    {
        int cx = cell.first;
        int cy = cell.second;
        if (!m_testMap.InBounds(cx, cy)) continue;
        sf::RectangleShape rect(sf::Vector2f(m_testMap.tileSize, m_testMap.tileSize));
        rect.setPosition(sf::Vector2f(cx * m_testMap.tileSize, cy * m_testMap.tileSize));
        rect.setFillColor(sf::Color(0, 0, 255, 60));
        rect.setOutlineColor(sf::Color::Blue);
        rect.setOutlineThickness(1.0f);
        m_window.draw(rect);
    }
}


// Draw a preview line during mouse drag to show the current ray segment being defined by the drag. This is drawn in green and only shown when the preview is active and visual debug mode is enabled.
void TileMapScene::DrawPreviewLine()
{
    if (!(m_previewActive && m_visualDebug)) return;
    sf::Vertex line[] =
    {
        sf::Vertex(sf::Vector2f(m_previewLine.first.x, m_previewLine.first.y), sf::Color::Green),
        sf::Vertex(sf::Vector2f(m_previewLine.second.x, m_previewLine.second.y), sf::Color::Green)
    };
    m_window.draw(line, 2, sf::PrimitiveType::Lines);
}


// HandleEvent now delegates to input helpers to keep logic centralized
void TileMapScene::HandleEvent(const std::optional<sf::Event>& event)
{
    //(void)event;
    // get mouse pixel coords then convert to world coords using the window's view
    sf::Vector2i mousePixelPos = sf::Mouse::getPosition(m_window);
    sf::Vector2f mouseWorldF = m_window.mapPixelToCoords(mousePixelPos);
    Vec2 mouseWorld(static_cast<float>(mouseWorldF.x), static_cast<float>(mouseWorldF.y));

    bool mouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    bool keyDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape);

    ProcessDebugToggle();
    ProcessMouseDrag(mouseDown, mouseWorld);
    ProcessKeyInput(keyDown);
}


// OnEnter method - currently empty, but could be used for setup logic that needs to run when the scene becomes active
void TileMapScene::OnEnter()
{
}


// OnExit method - currently empty, but could be used for cleanup logic that needs to run when the scene is no longer active
void TileMapScene::OnExit()
{
}


// LoadResources method - currently empty, but could be used to load textures, sounds, or other resources needed by the scene. In this example, we load the tile map in InitializeGame instead, but we could also move it here if we wanted to separate resource loading from game initialization.
void TileMapScene::LoadResources()
{
}


// Helper: create a synthetic RaycastHit representing an immediate hit at the start cell
RaycastHit TileMapScene::MakeStartCellHit(int tileX, int tileY, const Vec2& origin)
{
    RaycastHit h;
	if (tileX < 0 || tileY < 0) return h; // Guard: invalid cell, return no hit
    
	// We assume the caller has already checked that the start cell is solid. We create a hit at the origin with distance 0 and the tile value for diagnostics.
    h.hit = true;
    h.tileX = tileX;
    h.tileY = tileY;
    h.tileValue = m_testMap.GetTile(tileX, tileY);
    h.position = origin;
    h.distance = 0.0f;
    return h;
}


// UnloadResources method - currently empty, but could be used to free textures, sounds, or other resources when the scene is unloaded
void TileMapScene::UnloadResources()
{
}


// InitializeGame method - loads the tile map from a JSON file and creates a tile map entity in the entity manager. Errors are ignored or could be handled via UI, and there is no console output in this method.
void TileMapScene::InitializeGame(sf::Vector2u /*windowSize*/)
{
    // Load tilemap (no console output; errors are ignored or can be handled via UI)
    std::string err;
    auto maybe = LoadTileMapJSON("assets\\testmap.json", &err);
    if (maybe) { m_testMap = *maybe; m_entityManager->CreateTileMapEntity(m_testMap); }
}


// SpawnTestTileMap method - creates a simple test tile map with various platforms and obstacles for testing raycasting and visualization. This method is not currently called, but can be used to generate a procedural tile map instead of loading from JSON.
void TileMapScene::SpawnTestTileMap()
{
    // Create a tilemap sized to at least cover the window so platforms span the screen
    const float tileSize = 32.0f;
    const auto winSz = m_window.getSize();
    int cols = static_cast<int>(std::max<unsigned int>(20u, winSz.x / static_cast<unsigned int>(tileSize) + 2u));
    int rows = static_cast<int>(std::max<unsigned int>(15u, winSz.y / static_cast<unsigned int>(tileSize) + 2u));

    TileMap map(cols, rows, tileSize);

    // ground platform across the bottom
    int groundY = rows - 2;
    for (int x = 0; x < cols; ++x) map.SetTile(x, groundY, 1);

    // left and right vertical walls
    for (int y = groundY - 10; y <= groundY; ++y) {
        if (y >= 0 && y < rows) map.SetTile(2, y, 1);
        if (y >= 0 && y < rows) map.SetTile(cols - 3, y, 1);
    }

    // several floating platforms across the level
    for (int x = 6; x + 6 < cols; x += 12)
    {
        int py = std::max(2, groundY - 4 - ((x / 12) % 4));
        for (int lx = 0; lx < 6; ++lx) map.SetTile(x + lx, py, 1);
    }

    // a few clusters and obstacles
    for (int y = groundY - 6; y < groundY - 3; ++y)
        for (int x = cols/2 - 2; x <= cols/2 + 2; ++x)
            map.SetTile(x, y, 1);

    map.SetTile(4, 4, 1);
    map.SetTile(5, 4, 1);
    map.SetTile(cols - 6, 6, 1);
    map.SetTile(cols - 7, 6, 1);

    // additional walls/shafts spaced across the level
    for (int x = 8; x + 8 < cols; x += 16)
    {
        for (int y = groundY - 1; y >= std::max(2, groundY - 8); --y)
        {
            if (y >= 0 && y < rows) map.SetTile(x, y, 1);
        }
    }

    // staggered small platforms
    for (int i = 0; i < cols / 10; ++i)
    {
        int px = 4 + i * 10;
        int py = std::max(3, groundY - 3 - (i % 5));
        for (int w = 0; w < 4; ++w)
        {
            if (px + w < cols) map.SetTile(px + w, py, 1);
        }
    }

    // short ceiling platforms and small pillars
    for (int x = 10; x + 3 < cols - 10; x += 20)
    {
        for (int w = 0; w < 3; ++w) map.SetTile(x + w, 3, 1);
    }

    for (int x = 3; x + 3 < cols - 3; x += 14)
    {
        if (groundY - 1 >= 0) map.SetTile(x, groundY - 1, 1);
        if (groundY - 2 >= 0) map.SetTile(x, groundY - 2, 1);
    }

    // store and create entity
    m_testMap = map;
    m_entityManager->CreateTileMapEntity(map);
}


// Toggle a tile at the specified map cell coordinates (tx, ty). This method toggles the tile value between 0 and 1, updates the tile map in the entity manager, and marks it as dirty for rendering. This allows for interactive editing of the tile map by clicking on cells.
void TileMapScene::ToggleTileAt(int tx, int ty)
{
    if (!m_testMap.InBounds(tx, ty)) return;
    int value = m_testMap.GetTile(tx, ty);
    // Toggle between 0 and 1
    m_testMap.SetTile(tx, ty, value == 0 ? 1 : 0);
    // Mark entity manager tilemap dirty by creating/updating the component entity
    if (m_entityManager)
    {
        // Try to find an existing CTileMap component and update it
        bool updated = false;

		// Get the entities from the entity manager and look for one with a CTileMap component. If found, update its map reference and mark it dirty. We also remove any existing tile entities so that the TileSystem will recreate them based on the u
        for (auto& up_entity: m_entityManager->getEntities())
        {
            Entity* entity = up_entity.get();
            if (!entity) continue;
            auto comp = entity ->GetComponent<CTileMap>();
            if (comp)
            {
                comp->map = m_testMap;
                comp->m_dirty = true;
                m_entityManager->SetHasPendingTileMaps(true);
                // Remove any existing generated tile entities so TileSystem can recreate them from the updated map
                for (Entity* te : m_entityManager->getEntities(EntityType::Tile))
                {
                    if (te) m_entityManager->KillEntity(te);
                }
                updated = true;
                break;
            }
        }
        if (!updated)
        {
            // fallback: create a new tilemap entity
            // remove any existing tile entities first
            for (Entity* te : m_entityManager->getEntities(EntityType::Tile))
            {
                if (te) m_entityManager->KillEntity(te);
            }
            m_entityManager->CreateTileMapEntity(m_testMap);
        }
        // Recreate tile entities immediately from the updated map so visuals match state
        m_entityManager->AddTileMapAsEntities(m_testMap);
    }
}
