// TileMapScene.h - Simple scene to test tilemap component and TileSystem
#pragma once
#include "Scene.h"
#include "Raycast.h"
#include <vector>


class TileMapScene : public Scene
{
public:
    TileMapScene(GameEngine& engine, sf::RenderWindow& win);
    ~TileMapScene() override;

    void Update(float deltaTime) override;
    void Render() override;
    void DoAction() override;

    void HandleEvent(const sf::Event& event) override;
    void OnEnter() override;
    void OnExit() override;
    void LoadResources() override;
    void UnloadResources() override;
    void InitializeGame(sf::Vector2u windowSize) override;

private:
    void SpawnTestTileMap();
    sf::RenderWindow& m_window; // reference passed in constructor for render/context
    std::vector<std::pair<Vec2, Vec2>> m_debugLines; // lines to draw for raycasts
    std::vector<Vec2> m_debugPoints; // hit points
    std::vector<Vec2> m_rawHitPoints; // raw hit positions (before clamping) for debug
    // Mouse drag state for interactive raycasts
    bool m_dragging = false;
    Vec2 m_dragStart = Vec2(0,0);
    Vec2 m_dragEnd = Vec2(0,0);
    bool m_previewActive = false;
    std::pair<Vec2, Vec2> m_previewLine;
    TileMap m_testMap;
    bool m_prevMouseDown = false;
    // last cast start tile (for visual highlight)
    int m_lastStartTileX = -1;
    int m_lastStartTileY = -1;
    bool m_lastStartSolid = false;
    std::vector<std::pair<int,int>> m_visitedCells; // cells visited by last DDA cast
    std::vector<std::pair<int,int>> m_visitedIgnore; // visited cells when ignoring start cell
};
