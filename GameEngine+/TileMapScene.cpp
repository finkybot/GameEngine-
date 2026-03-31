#include "TileMapScene.h"
#include "GameEngine.h"
#include "EntityManager.h"
#include "CTileMap.h"
#include <iostream>
#include "Raycast.h"
#include <SFML/Graphics/RectangleShape.hpp>
#include <Utils.h>

TileMapScene::TileMapScene(GameEngine& engine, sf::RenderWindow& win) : Scene(engine), m_window(win)
{
    EntityManager* em = new EntityManager(win);
    m_entityManager = em;
}

TileMapScene::~TileMapScene() = default;

void TileMapScene::Update(float /*deltaTime*/)
{
    // Handle events (SFML 3.0: pollEvent returns std::optional<sf::Event>)
    while (auto eventOpt = m_gameEngine.m_window.pollEvent())
    {
        // ImGui::SFML::ProcessEvent(m_gameEngine.m_window, *eventOpt);

        if (eventOpt->is<sf::Event::Closed>())
        {
            std::cout << "Window close event received. Closing the game." << std::endl;
            m_gameEngine.m_window.close();
        }
    }

    // simple update: run entity manager update
    m_entityManager->Update(0.016f);
    // Draw debug lines to the window (convert Vec2 to SFML lines)
    // We'll draw in Render() instead using m_debugLines stored by SpawnTestTileMap

}

void TileMapScene::Render()
{
    // draw tile grid (solid tiles)
    if (m_testMap.width > 0 && m_testMap.height > 0)
    {
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

    // draw debug ray lines
    for (const auto& pr : m_debugLines)
    {
        sf::Vertex line[] =
        {
            sf::Vertex(sf::Vector2f(pr.first.x, pr.first.y), sf::Color::Red),
            sf::Vertex(sf::Vector2f(pr.second.x, pr.second.y), sf::Color::Red)
        };
        m_window.draw(line, 2, sf::PrimitiveType::Lines);
    }

    // draw visited-ignore cells overlay in a different color
    for (const auto& cell : m_visitedIgnore)
    {
        int cx = cell.first;
        int cy = cell.second;
        if (!m_testMap.InBounds(cx, cy)) continue;
        sf::RectangleShape rect(sf::Vector2f(m_testMap.tileSize, m_testMap.tileSize));
        rect.setPosition(sf::Vector2f(cx * m_testMap.tileSize, cy * m_testMap.tileSize));
        rect.setFillColor(sf::Color(255, 0, 255, 40));
        rect.setOutlineColor(sf::Color::Magenta);
        rect.setOutlineThickness(1.0f);
        m_window.draw(rect);
    }

    // draw hit points
    for (const auto& p : m_debugPoints)
    {
        sf::CircleShape dot(4.0f);
        dot.setFillColor(sf::Color::Yellow);
        dot.setOrigin(sf::Vector2f(4.0f, 4.0f));
        dot.setPosition(sf::Vector2f(p.x, p.y));
        m_window.draw(dot);
    }

    // draw raw hit points in blue (before clamping)
    for (const auto& p : m_rawHitPoints)
    {
        sf::CircleShape dot(3.0f);
        dot.setFillColor(sf::Color::Blue);
        dot.setOrigin(sf::Vector2f(3.0f, 3.0f));
        dot.setPosition(sf::Vector2f(p.x, p.y));
        m_window.draw(dot);
    }

    // draw visited cells overlay
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

    // draw preview line if active
    if (m_previewActive)
    {
        sf::Vertex line[] =
        {
            sf::Vertex(sf::Vector2f(m_previewLine.first.x, m_previewLine.first.y), sf::Color::Green),
            sf::Vertex(sf::Vector2f(m_previewLine.second.x, m_previewLine.second.y), sf::Color::Green)
        };
        m_window.draw(line, 2, sf::PrimitiveType::Lines);
    }
}

void TileMapScene::DoAction()
{
}

void TileMapScene::HandleEvent(const sf::Event& event)
{
    // Fallback: poll mouse state directly using SFML window (works across SFML versions)
    // We'll use real-time mouse state to implement drag/preview and only use events for other things.
    (void)event;
    // get mouse pixel coords then convert to world coords using the window's view
    sf::Vector2i mousePixelPos = sf::Mouse::getPosition(m_window);
    sf::Vector2f mouseWorldF = m_window.mapPixelToCoords(mousePixelPos);
    Vec2 mouseWorld(static_cast<float>(mouseWorldF.x), static_cast<float>(mouseWorldF.y));
    bool mouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	bool keyDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape);

    if (mouseDown && !m_prevMouseDown)
    {
        m_dragging = true;
        // use world coordinates for ray origin so it aligns with the tilemap/world
        m_dragStart = mouseWorld;
        m_previewActive = true;
    }
    else if (!mouseDown && m_prevMouseDown)
    {
        if (m_dragging)
        {
            m_dragging = false;
            m_dragEnd = mouseWorld;
            Vec2 dir = m_dragEnd - m_dragStart;
            float dragLen = dir.Mag();
            std::cout << "Mouse drag length=" << dragLen << " dir(" << dir.x << "," << dir.y << ")\n";
            if (dragLen <= 0.001f)
            {
                m_prevMouseDown = mouseDown;
                return;
            }
            dir = dir.GetUnitVec();

            // Use the drag length as the raycast maximum distance so hits are limited to the drag segment
            // Clear previous debug visuals for clarity
            m_debugLines.clear();
            m_debugPoints.clear();

            // Compute starting cell and log diagnostics
            int startTileX = static_cast<int>(std::floor(m_dragStart.x / m_testMap.tileSize));
            int startTileY = static_cast<int>(std::floor(m_dragStart.y / m_testMap.tileSize));
            bool startSolid = m_testMap.InBounds(startTileX, startTileY) && m_testMap.IsSolid(startTileX, startTileY);
            std::cout << "StartTile=(" << startTileX << "," << startTileY << ") solid=" << startSolid << " startPos=(" << m_dragStart.x << "," << m_dragStart.y << ")\n";

            // Try DDA both ways for diagnostics: not ignoring start, and ignoring start when appropriate
            m_visitedCells.clear();
            RaycastHit rh_force = RaycastTilemapDDA(m_dragStart, dir, m_testMap, dragLen, false, &m_visitedCells);
            m_visitedIgnore.clear();
            RaycastHit rh_ignore = RaycastTilemapDDA(m_dragStart, dir, m_testMap, dragLen, startSolid, &m_visitedIgnore);

            // Build a stable visited-cell list by sampling the segment at fixed intervals.
            // This is deterministic and avoids DDA edge-case visualization gaps.
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
            std::cout << "DDA forceStartHit=" << (rh_force.hit?"true":"false") << " tile=(" << rh_force.tileX << "," << rh_force.tileY << ") dist=" << rh_force.distance << "\n";
            std::cout << "DDA ignoreStartHit=" << (rh_ignore.hit?"true":"false") << " tile=(" << rh_ignore.tileX << "," << rh_ignore.tileY << ") dist=" << rh_ignore.distance << "\n";

            // Use the conditional logic: if start is solid, report the start-cell hit but also check for a further hit
            RaycastHit rh;
            if (startSolid)
            {
                rh = rh_force; // report immediate hit at start cell
                // if there is also a further hit beyond the start cell, keep it for diagnostics/visualization
                if (rh_ignore.hit && (rh_ignore.tileX != rh_force.tileX || rh_ignore.tileY != rh_force.tileY))
                {
                    // show the further hit as an additional debug line/point (blue)
                    m_debugLines.push_back({ m_dragStart, rh_ignore.position });
                    m_rawHitPoints.push_back(rh_ignore.position);
                    std::cout << "Also found further hit beyond start at tile (" << rh_ignore.tileX << "," << rh_ignore.tileY << ") dist=" << rh_ignore.distance << "\n";
                }
            }
            else
            {
                rh = rh_ignore;
            }
            if (rh.hit)
            {
                // Clamp hit to the drag segment in case of numerical/rounding differences
                Vec2 hitPos = rh.position;
                float proj = (hitPos.x - m_dragStart.x) * dir.x + (hitPos.y - m_dragStart.y) * dir.y; // scalar along dir
                if (proj > dragLen)
                {
                    hitPos = Vec2(m_dragStart.x + dir.x * dragLen, m_dragStart.y + dir.y * dragLen);
                    std::cout << "Clamped hit pos to drag end\n";
                }
                m_debugLines.push_back({ m_dragStart, hitPos });
                m_debugPoints.push_back(hitPos);
                m_rawHitPoints.push_back(rh.position);
                std::cout << "Mouse raycast hit tile (" << rh.tileX << "," << rh.tileY << ") at world pos (" << rh.position.x << "," << rh.position.y << ") dist=" << rh.distance << " val=" << rh.tileValue << "\n";
            }
            else
            {
                // print starting tile cell and start position for debugging
                int startTileX = static_cast<int>(std::floor(m_dragStart.x / m_testMap.tileSize));
                int startTileY = static_cast<int>(std::floor(m_dragStart.y / m_testMap.tileSize));
                std::cout << "Mouse raycast NO HIT within drag length=" << dragLen << "; start cell=(" << startTileX << "," << startTileY << ") startPos=(" << m_dragStart.x << "," << m_dragStart.y << ")\n";
                // show the attempted cast segment so the user can see alignment
                m_debugLines.push_back({ m_dragStart, m_dragEnd });
                m_debugPoints.push_back(m_dragEnd);
                // Diagnostic: sample along the drag segment to see if any tile appears solid
                bool sampledHit = false;
                float step = std::max(1.0f, m_testMap.tileSize * 0.25f);
                for (float t = 0.0f; t <= dragLen; t += step)
                {
                    Vec2 p(m_dragStart.x + dir.x * t, m_dragStart.y + dir.y * t);
                    int cx = static_cast<int>(std::floor(p.x / m_testMap.tileSize));
                    int cy = static_cast<int>(std::floor(p.y / m_testMap.tileSize));
                    if (m_testMap.InBounds(cx, cy) && m_testMap.IsSolid(cx, cy))
                    {
                        std::cout << "Diagnostic: sampled point at t=" << t << " hits solid tile (" << cx << "," << cy << ") worldPos=(" << p.x << "," << p.y << ")\n";
                        sampledHit = true;
                        break;
                    }
                }
                if (!sampledHit)
                {
                    std::cout << "Diagnostic: sampled along ray found no solid tiles between start and end.\n";
                    // additional diagnostic: print start/end tile and DDA precomputed values
                    int endTileX = static_cast<int>(std::floor(m_dragEnd.x / m_testMap.tileSize));
                    int endTileY = static_cast<int>(std::floor(m_dragEnd.y / m_testMap.tileSize));
                    std::cout << "Start cell=(" << startTileX << "," << startTileY << ") End cell=(" << endTileX << "," << endTileY << ")\n";

                    float invTile = 1.0f / m_testMap.tileSize;
                    Vec2 originG(m_dragStart.x * invTile, m_dragStart.y * invTile);
                    Vec2 dirG(dir.x * invTile, dir.y * invTile);
                    std::cout << "originG=(" << originG.x << "," << originG.y << ") dirG=(" << dirG.x << "," << dirG.y << ") maxDistGrid=" << dragLen * invTile << "\n";
                }
            }
        }
        m_previewActive = false;
    }
    else if (mouseDown && m_dragging)
    {
        m_dragEnd = mouseWorld;
        m_previewLine = { m_dragStart, m_dragEnd };
    }

    m_prevMouseDown = mouseDown;

    if (keyDown)
    {
        std::cout << "Escape key pressed. Closing the game." << std::endl;
        m_gameEngine.m_window.close();
	}
}

void TileMapScene::OnEnter()
{
}

void TileMapScene::OnExit()
{
}

void TileMapScene::LoadResources()
{
}

void TileMapScene::UnloadResources()
{
}

void TileMapScene::InitializeGame(sf::Vector2u windowSize)
{
	std::cout << "Initializing TileMapScene with window size: " << windowSize.x << "x" << windowSize.y << std::endl;
   // SpawnTestTileMap();
   
    
    
    std::string err;
    auto maybe = LoadTileMapJSON("assets\\testmap.json", &err);


   //  if (!SaveTileMapJSON(m_testMap, "assets\\testmap.json", &err))
   //   {
   //       std::cerr << "Error saving tilemap to JSON: " << err << std::endl;
   //   }

    if (maybe) { m_testMap = *maybe; m_entityManager->CreateTileMapEntity(m_testMap); }
    else std::cerr << err << '\n';
}

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
