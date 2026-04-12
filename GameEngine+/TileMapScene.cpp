#include "TileMapScene.h"
#include "GameEngine.h"
#include "EntityManager.h"
#include "CTileMap.h"
#include <iostream>
#include "Raycast.h"
#include "TileMap.h"
#include <SFML/Graphics/RectangleShape.hpp>
#include <Utils.h>
#include "Vec2.h"
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Event.hpp>
#include "Entity.h"
#include "CText.h"


// Constructor - initializes the tile map scene with a reference to the game engine and the render window, and sets up the entity manager
TileMapScene::TileMapScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager) : Scene(engine, entityManager), m_window(win)
{
}


// Destructor - defaulted since we don't have any special cleanup logic, but we could add it if needed in the future
TileMapScene::~TileMapScene() = default;

// Process left control key state to enable/disable
void TileMapScene::ProcessLeftCtrlKey(bool keyDown)
{
	m_leftCtrlKeyDown = keyDown;
}

// Process debug toggle key (D) to show/hide visual debug overlays
void TileMapScene::ProcessDebugToggle(bool debugToggle)
{
    if (debugToggle && !m_prevDebugKeyDown) m_visualDebug = !m_visualDebug;
    m_prevDebugKeyDown = debugToggle;
}


// Helper to create a RaycastHit when ray starts inside a solid tile, since DDA will return no hit in this case
void TileMapScene::ProcessMouseDragRaycast(bool leftMouseDown, const Vec2& mouseWorld)
{
	// Handle mouse drag state transitions and perform raycast on release
	if (leftMouseDown && !m_prevLmbMouseDown) 
    {
		m_lmbdragging = true;                           
		m_lmbDragStart = mouseWorld;                    
		m_previewActive = true;                         
    } 
    // If mouse is up and was down in previous frame, end dragging and perform raycast
	else if (!leftMouseDown && m_prevLmbMouseDown) 
    {
		// Only perform raycast if we were dragging, otherwise it was just a click without movement, and we will handle that case separately to toggle tile state. This prevents us from performing unnecessary raycasts on simple clicks 
		// and allows us to use clicks for toggling tiles while still supporting drag-based raycasts for longer drags. We get the magnitude of the drag and if it's very small, we treat it as a click rather than a drag, which allows us 
        // to toggle the tile state without performing a raycast. This provides a more intuitive user experience where clicks can be used for quick edits and drags can be used for raycasting.
		if (m_lmbdragging) 
        {
            m_lmbdragging = false;                      
            m_lmbDragEnd = mouseWorld;                      
			Vec2 dir = m_lmbDragEnd - m_lmbDragStart;   
			float dragLen = dir.Mag();                  

			// If the drag length is very small, we can treat it as a click rather than a drag. We use this to toggle a tile solid/not solid state. 
            // I'm using a very small threshold of 0.001f but it can be adjusted based on testing and needs.
            if (dragLen <= 0.001f)              
            {                
				m_previewActive = false;                
				m_prevLmbMouseDown = leftMouseDown;     
				return;                                 
            }
           
			// Normalize the direction vector for raycasting. We will use the original drag length to clamp the raycast distance, but the direction needs to be a unit vector for the DDA algorithm.
			// Also clear previous debug lines and points before performing the new raycast, so that we only visualize the current raycast result. This ensures that our debug visualization is accurate and up-to-date with the latest raycast.
			dir = dir.GetUnitVec();                    
			m_debugLines.clear();                      
			m_debugPoints.clear();

			// Determine if the starting cell is solid, which affects how we handle the raycast. If the ray starts inside a solid tile, the DDA will not report a hit since it only detects transitions from empty to solid. 
            // We need to check the starting cell and if it's solid, we will create a synthetic hit at the start position to represent the immediate collision. This allows us to visualize rays that start inside walls and 
            // ensures consistent behavior for clicks and short drags that begin in solid tiles.
			int startTileX = static_cast<int>(std::floor(m_lmbDragStart.x / m_tileMap.tileSize));                           
            int startTileY = static_cast<int>(std::floor(m_lmbDragStart.y / m_tileMap.tileSize));                           
			bool startSolid = m_tileMap.InBounds(startTileX, startTileY) && m_tileMap.IsSolid(startTileX, startTileY);      

			// Perform the raycast using DDA. If the start cell is solid, we will ignore it in the DDA and create a synthetic hit at the start position. The DDA will then find 
            // the next hit along the ray, which allows us to visualize rays that start inside walls and see where they exit or hit another wall.
			m_visitedCells.clear(); // clear visited cells before DDA
			RaycastHit rayHitIgnore = RaycastTilemapDDA(m_lmbDragStart, dir, m_tileMap, dragLen, startSolid, &m_visitedCells); 
			RaycastHit rayHitStartCell = MakeStartCellHit(startTileX, startTileY, m_lmbDragStart);                                

			// Build the list of visited cells for visualization. We will sample points along the ray at regular intervals and determine which cells they correspond to, adding them to the visited cells list if they are not already in it. 
            // This allows us to visualize the path of the ray through the grid and see which cells it passes through, which can be helpful for debugging the raycasting logic and understanding how rays interact with the tilemap.
			m_visitedCells.clear();                                    
            float step = std::max(1.0f, m_tileMap.tileSize * 0.25f);
            
			// Sample points along the ray at regular intervals and determine which cells they correspond to, adding them to the visited cells list if they are not already in it. 
            // This allows us to visualize the path of the ray through the grid and see which cells it passes through, which can be helpful for debugging the raycasting logic and understanding how rays interact with the tilemap.
            for (float rayDistance = 0.0f; rayDistance <= dragLen; rayDistance += step)           
            {
				Vec2 worldPos(m_lmbDragStart.x + dir.x * rayDistance, m_lmbDragStart.y + dir.y * rayDistance);
                int cellX = static_cast<int>(std::floor(worldPos.x / m_tileMap.tileSize));
                int cellY = static_cast<int>(std::floor(worldPos.y / m_tileMap.tileSize));
                
				// Add the cell to the visited cells list if it's not already in it. We check if the cell is within bounds of the tilemap before adding it, and we also check if it's
                // already in the visited cells list to avoid duplicates. This allows us to visualize which cells were visited during the raycast.
                if (!m_tileMap.InBounds(cellX, cellY)) continue;
                if (std::find(m_visitedCells.begin(), m_visitedCells.end(), std::pair<int, int>(cellX, cellY)) == m_visitedCells.end()) { m_visitedCells.push_back({ cellX, cellY }); }
            }

			// Determine which hit to use for visualization. If the ray starts inside a solid tile, we will use the synthetic hit at the start position for visualization, but we will also check if the DDA reported a 
            // different hit further along the ray. If it did, we will draw a line to that hit as well to show the exit point from the wall. If the ray starts in an empty tile, we will just use the DDA hit as normal.
            RaycastHit rayHit;
            if (startSolid)
            {
				// If the ray starts inside a solid tile, we use the synthetic hit at the start position for visualization. However, we also check if the DDA reported a different hit further along the ray. 
                // If it did, we will draw a line to that hit as well to show the exit point from the wall. This allows us to visualize rays that start inside walls and see where they exit or hit another wall.
                rayHit = rayHitStartCell;  
                if (rayHitIgnore.hit && (rayHitIgnore.tileX != rayHitStartCell.tileX || rayHitIgnore.tileY != rayHitStartCell.tileY))
                {
                    m_debugLines.push_back({ m_lmbDragStart, rayHitIgnore.position });
                    m_rawHitPoints.push_back(rayHitIgnore.position);
                }
            }
            // Otherwise, if the ray starts in an empty tile, we just use the DDA hit as normal for visualization.
			else 
            {
                rayHit = rayHitIgnore;
            }

			// For visualization, we will draw a line from the drag start to the hit position. If there was a hit, we will clamp the hit position to the ray length in case it exceeds it (which can happen if the ray starts inside 
            // a solid tile and the DDA reports a hit at the boundary). We will also draw a point at the hit position. If there was no hit, we will draw a line to the drag end position and a point there instead. This allows us 
            // to visualize the result of the raycast and see where it hit or where it ended if it didn't hit anything.
            if (rayHit.hit)
            {
                Vec2 hitPos = rayHit.position;
                float proj = (hitPos.x - m_lmbDragStart.x) * dir.x + (hitPos.y - m_lmbDragStart.y) * dir.y;
                if (proj > dragLen)
                {
                    hitPos = Vec2(m_lmbDragStart.x + dir.x * dragLen, m_lmbDragStart.y + dir.y * dragLen);
                }
                m_debugLines.push_back({ m_lmbDragStart, hitPos });
                m_debugPoints.push_back(hitPos);
                m_rawHitPoints.push_back(rayHit.position);
            }
			// Otherwise, if there was no hit, we will draw a line to the drag end position and a point there instead. This allows us to visualize the result of the raycast and see where it hit or where it ended if it didn't hit anything.
            else
            {
                m_debugLines.push_back({ m_lmbDragStart, m_lmbDragEnd });
                m_debugPoints.push_back(m_lmbDragEnd);
            }

            m_previewActive = false;
        }
    }
	// Only update the preview line if we are currently dragging with the left mouse button. This allows us to show a real-time preview of the raycast as we drag, 
    // which can be helpful for aiming and visualizing where the ray will go before we release the mouse button to perform the actual raycast.
    else if (leftMouseDown && m_lmbdragging)
    {
        m_lmbDragEnd = mouseWorld;
        m_previewLine = { m_lmbDragStart, m_lmbDragEnd };
    }

    m_prevLmbMouseDown = leftMouseDown;
}


// Process right mouse drag to toggle tiles. We will toggle the tile state at the current mouse position when the right mouse button is pressed, and we will also support dragging to toggle multiple tiles in a single drag. 
// When the right mouse button is released, we will end the drag. This allows us to quickly edit the tilemap by clicking or dragging with the right mouse button to toggle tiles between solid and empty states.
void TileMapScene::ProcessMouseRightDrag(bool& rightMouseDown, const Vec2& mouseWorld)
{
	Vec2 currentTile = Vec2(static_cast<int>(std::floor(mouseWorld.x / m_tileMap.tileSize)), static_cast<int>(std::floor(mouseWorld.y / m_tileMap.tileSize)));

	// If the right mouse button is pressed and was not pressed in the previous frame, we start a new drag operation. We also check if the current tile under the mouse is different from the last tile we toggled 
    // to prevent multiple toggles on the same tile when we first press down. This allows us to toggle a tile immediately on right-click without dragging, while still supporting dragging to toggle multiple tiles.
    if (rightMouseDown && !m_prevRmbMouseDown && currentTile != m_currentTile)
    {
        m_rmbdragging = true;           
        m_rmbDragStart = mouseWorld;
		m_currentTile = currentTile; // Set the current tile to the tile under the mouse when we start dragging, so that we can track which tile we last toggled and avoid toggling the same tile multiple times on click.
	    ToggleTileAt(currentTile.x, currentTile.y);
    }
	// Otherwise if the right mouse button is released and was pressed in the previous frame, we end the drag operation. If we were dragging, we record the drag end position 
    // and calculate the drag direction vector, although for tile toggling we may not need the direction.
    else if (!rightMouseDown && m_prevRmbMouseDown) 
    {
		// Only end dragging if we were actually dragging, otherwise it was just a click without movement, and we will handle that case separately to toggle tile state. 
        // This prevents us from performing unnecessary toggles on simple clicks
        if (m_rmbdragging)
        {            
            m_rmbdragging = false;                      
            m_rmbDragEnd = mouseWorld;                      
            Vec2 dir = m_rmbDragEnd - m_rmbDragStart;   
           // if (currentTile != m_currentTile) ToggleTileAt(currentTile.x, currentTile.y); // Toggle the tile at the current mouse position on release if it's different from the last toggled tile, to ensure we toggle the tile on click as well as on drag.

			m_currentTile = Vec2(static_cast<int>(std::floor(m_rmbDragStart.x / m_tileMap.tileSize)), static_cast<int>(std::floor(m_rmbDragStart.y / m_tileMap.tileSize)));
            //ToggleTileAt(currentTile.x, currentTile.y);
        }
    }
	// If the right mouse button is currently down and we are in a dragging state, we will update the drag end position and toggle the tile at the current mouse position if it's different from the last toggled tile.
	else if (rightMouseDown && m_rmbdragging && currentTile != m_currentTile)
    {
        m_rmbDragEnd = mouseWorld;
		m_currentTile = Vec2(static_cast<int>(std::floor(m_rmbDragEnd.x / m_tileMap.tileSize)), static_cast<int>(std::floor(m_rmbDragEnd.y / m_tileMap.tileSize)));
        ToggleTileAt(currentTile.x, currentTile.y);
    }
	// Additionally, we want to support toggling tiles on simple right-clicks without dragging. To do this, we will check if the right mouse button is currently down and was not down in the previous frame (indicating a new click), 
    // and if the current tile under the mouse is the same as the last toggled tile, we will toggle it again. This allows us to toggle a tile immediately on right-click without dragging, while still supporting dragging to toggle multiple tiles.
    else if (rightMouseDown && !m_prevRmbMouseDown && currentTile == m_currentTile)
    {
        ToggleTileAt(currentTile.x, currentTile.y);
	}


    m_prevRmbMouseDown = rightMouseDown;
}


// Process key input to close the window when any key is pressed
void TileMapScene::ProcessEscapeKey(bool keyDown) const { if (keyDown) m_gameEngine.m_window.close(); }

void TileMapScene::ProcessSaveKey(bool keyDown) const
{
    if (keyDown && m_leftCtrlKeyDown)
    {
        std::string filename = "assets//testmap.json";
        std::string err;
        if (!m_tileMap.SaveToJSON(filename, &err))
        {
			std::cerr << "Error saving tilemap: " << err << std::endl;
        }
        else
        {
            std::cout << "Tilemap saved to " << filename << std::endl;
        }
    }
}


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
    //m_entityManager.Update(0.016f);
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

    // Text is rendered by the RenderSystem during EntityManager::Update; no per-scene text draw here to avoid double-rendering.
}


// DoAction method - currently empty, but could be used for game logic that needs to run on a fixed timestep or in response to certain conditions
void TileMapScene::DoAction()
{
}


// Draw the tile grid by iterating over the tile map and drawing a semi-transparent rectangle for each solid tile
void TileMapScene::DrawTileGrid()
{
    if (m_tileMap.width <= 0 || m_tileMap.height <= 0) return;
    for (int y = 0; y < m_tileMap.height; ++y)
    {
        for (int x = 0; x < m_tileMap.width; ++x)
        {
            if (m_tileMap.IsSolid(x, y))
            {
                sf::RectangleShape rect(sf::Vector2f(m_tileMap.tileSize, m_tileMap.tileSize));
                rect.setPosition(sf::Vector2f(x * m_tileMap.tileSize, y * m_tileMap.tileSize));
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
        if (!m_tileMap.InBounds(cx, cy)) continue;
        sf::RectangleShape rect(sf::Vector2f(m_tileMap.tileSize, m_tileMap.tileSize));
        rect.setPosition(sf::Vector2f(cx * m_tileMap.tileSize, cy * m_tileMap.tileSize));
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

    bool leftMouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	bool rightMouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);
    
    bool leftCtrlKeyDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl);
    bool escapeKeyDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape);
    bool debugToggle = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
	bool saveKeyDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);

	ProcessLeftCtrlKey(leftCtrlKeyDown);
    ProcessDebugToggle(debugToggle);
    ProcessMouseDragRaycast(leftMouseDown, mouseWorld);
	ProcessMouseRightDrag(rightMouseDown, mouseWorld);
    ProcessEscapeKey(escapeKeyDown);
	ProcessSaveKey(saveKeyDown);
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


// UnloadResources method - currently empty, but could be used to free textures, sounds, or other resources when the scene is unloaded
void TileMapScene::UnloadResources()
{
}


// InitializeGame method - loads the tile map from a JSON file and creates a tile map entity in the entity manager. Errors are ignored or could be handled via UI, and there is no console output in this method.
void TileMapScene::InitializeGame(sf::Vector2u /*windowSize*/)
{
	sf::Vector2u windowSize = m_window.getSize(); // Get the actual window size for use in tile map loading and entity setup

    // Load tilemap (no console output; errors are ignored or can be handled via UI)
    std::string err;
    auto maybe = TileMap::LoadFromJSON("assets\\testmap.json", &err);
    if (maybe) { m_tileMap = *maybe; m_entityManager.CreateTileMapEntity(m_tileMap); }


    if (!m_gameEngine.GetFontManager().LoadFont("regular", "assets\\fonts\\roboto\\Roboto-Regular.ttf")) std::cerr << "Error loading font" << std::endl;
    if (!m_gameEngine.GetFontManager().LoadFont("thin", "assets\\fonts\\roboto\\Roboto-Thin.ttf")) std::cerr << "Error loading font" << std::endl;

	// Create an entity and add a text component to it to display the scene name. This demonstrates how to create entities and add components in the entity manager.
    // We load a font and set the text to "TileMapScene Demo" with a size of 20 and white color. If the font fails to load, we print an error message to the console.
    Entity* fontEntity = m_entityManager.addEntity(EntityType::Default);
    
    fontEntity->AddComponent<CTransform>(Vec2(50, 50), Vec2::Zero);
	if(!fontEntity->AddComponent<CText>("RayCasting Demo, with TileMap \nUsing GameEngine+", sf::Color::Cyan, "regular", 60)) /* Handle error if needed */ std::cerr << "Error loading font for text entity" << std::endl;

    Entity* instructionsEntity = m_entityManager.addEntity(EntityType::Default);

    instructionsEntity->AddComponent<CTransform>(Vec2(50, windowSize.y - 150), Vec2::Zero);
    if(!instructionsEntity->AddComponent<CText>("Left Click + Drag: Raycast\nRight Click + Drag: Toggle Tiles\nPress 'D' to toggle debug visualization\nPress 'Ctrl+S' to save tilemap", sf::Color::Yellow, "thin", 20)) std::cerr << "Error loading font for instructions entity" << std::endl;
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
    m_tileMap = map;
    m_entityManager.CreateTileMapEntity(map);
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
    h.tileValue = m_tileMap.GetTile(tileX, tileY);
    h.position = origin;
    h.distance = 0.0f;
    return h;
}

// Toggle a tile at the specified map cell coordinates (tx, ty). This method toggles the tile value between 0 and 1, updates the tile map in the entity manager, and marks it as dirty for rendering. This allows for interactive editing of the tile map by clicking on cells.
void TileMapScene::ToggleTileAt(int tx, int ty)
{
    if (!m_tileMap.InBounds(tx, ty)) return;
    int value = m_tileMap.GetTile(tx, ty);
    // Toggle between 0 and 1
    m_tileMap.SetTile(tx, ty, value == 0 ? 1 : 0);
    // Mark entity manager tilemap dirty by creating/updating the component entity
    // (m_entityManager is a reference guaranteed to be valid)
    {
        // Try to find an existing CTileMap component and update it
        bool updated = false;

		// Get the entities from the entity manager and look for one with a CTileMap component. If found, update its map reference and mark it dirty. We also remove any existing tile entities so that the TileSystem will recreate them based on the u
        for (auto& up_entity: m_entityManager.getEntities())
        {
            Entity* entity = up_entity.get();
            if (!entity) continue;
            auto comp = entity ->GetComponent<CTileMap>();
            if (comp)
            {
                comp->map = m_tileMap;
                comp->m_dirty = true;
                m_entityManager.SetHasPendingTileMaps(true);
                updated = true;
                break;
            }
        }

        if (!updated)
        {
            // Create a new CTileMap entity so TileSystem can process it on the next update
            // mark existing tile entities for removal first
            for (Entity* te : m_entityManager.getEntities(EntityType::Tile))
            {
                if (te) m_entityManager.KillEntity(te);
            }
            m_entityManager.CreateTileMapEntity(m_tileMap);
            m_entityManager.SetHasPendingTileMaps(true);
        }
        // Do NOT recreate tile entities immediately here. Let TileSystem process the dirty CTileMap
        // during the EntityManager::Update cycle so dead tile entities are removed first and
        // duplicates / stale visuals do not remain.
    }
}
