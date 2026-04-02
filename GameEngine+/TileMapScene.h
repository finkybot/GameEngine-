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

    void HandleEvent(const std::optional<sf::Event>& event) override;
    void OnEnter() override;
    void OnExit() override;
    void LoadResources() override;
    void UnloadResources() override;
    void InitializeGame(sf::Vector2u windowSize) override;

private:
    void SpawnTestTileMap();
    // Render helpers
    void DrawTileGrid();
    void DrawDebugLines();
    void DrawHitPoints();
    void DrawRawHitPoints();
    void DrawVisitedCells();
    void DrawPreviewLine();
    
    // Input helpers
	void ProcessLeftCtrlKey(bool keyDown);
    void ProcessDebugToggle(bool keyDown);
    void ProcessEscapeKey(bool keyDown) const;
    void ProcessSaveKey(bool keyDown) const;

    void ProcessMouseDragRaycast(bool leftMouseDown, const Vec2& mouseWorld);
	void ProcessMouseRightDrag(bool &rightMouseDown, const Vec2& mouseWorld);

    
    sf::RenderWindow& m_window; // reference passed in constructor for render/context
    Vec2 m_currentTile; // 2D grid of tile values for rendering

    std::vector<std::pair<Vec2, Vec2>> m_debugLines; // lines to draw for raycasts
    std::vector<Vec2> m_debugPoints; // hit points
    std::vector<Vec2> m_rawHitPoints; // raw hit positions (before clamping) for debug
    
    // Mouse drag state for interactive raycasts
    bool m_lmbdragging = false;
	bool m_rmbdragging = false;

    Vec2 m_lmbDragStart = Vec2(0,0);
    Vec2 m_lmbDragEnd = Vec2(0,0);
	Vec2 m_rmbDragStart = Vec2(0, 0);
	Vec2 m_rmbDragEnd = Vec2(0, 0);

    bool m_previewActive = false;
    
    std::pair<Vec2, Vec2> m_previewLine;
    TileMap m_tileMap;
    
    bool m_prevLmbMouseDown = false;
    bool m_prevRmbMouseDown = false;
	bool m_leftCtrlKeyDown  = false;

    bool m_visualDebug = true; // runtime toggle for visual debug overlays
    bool m_prevDebugKeyDown = false; // previous state of debug toggle key
    
    // last cast start tile (for visual highlight)
    int m_lastStartTileX = -1;
    int m_lastStartTileY = -1;
    bool m_lastStartSolid = false;
    
    std::vector<std::pair<int,int>> m_visitedCells; // cells visited by last DDA cast
    // helper to synthesize a start-cell hit when ray begins inside a solid tile
    
    RaycastHit MakeStartCellHit(int tileX, int tileY, const Vec2& origin);
    // Additional comment for clarity
    
    
    // Toggle a tile at map cell (x,y)
    void ToggleTileAt(int tx, int ty);
};
