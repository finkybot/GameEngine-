# Copilot Instructions

## Project Guidelines
- Prefer ECS architecture and logic in systems.
- Avoid dereferencing pointers after unique_ptr deletion; use address-based removal in EntityManager::RemoveDeadEntities.
- When removing dead entities, collect dead pointers and remove references from secondary structures by pointer identity using an unordered_set before deleting unique_ptrs; avoid dereferencing raw pointers that may have been freed. Collect dead pointers then remove them from other structures by pointer comparison to avoid use-after-free.
- Defer processing (ProcessPending) to the main update to avoid re-entrancy.
- Render entity text after scene overlays; ensure that entity text is displayed on top of scene overlays for better visibility.
- Plan to create a full sprite system and combine tilemaps with textures; ensure that rendering logic accommodates both sprites and tilemaps effectively.
- Implement interactive audio-reactive shapes by spawning shape entities based on music amplitude/beat via ImGui in MusicVisualizerScene. **(Note: Strip music playback and audio-reactive effects out of TileMapEditorScene and keep them only in MusicVisualizerScene.)** Start implementing/experimenting with graphical effects in MusicVisualizerScene.
- Investigate the equalizer issue in MusicVisualizerScene where many bars show identical responses, likely due to MusicSystem providing only 10 spectrum bands. Consider increasing spectrum resolution and enhancing MusicSystem analysis.
- Implement chunking in a new scene using a chunked tilemap design with a default chunk size of 32x32 tiles:
  - Use ChunkManager/ChunkedTileMap and an LRU cache of loaded chunks.
  - Load/save per-chunk JSON asynchronously; finalize render data on the main thread, mark dirty chunks, and save on eviction or explicit save.
  - Do not store entity IDs in Chunk struct for now; omit entityIds until needed.
  - Integrate this with TileMapEditorScene.
- Edit the editor cursor and provide different sf::Cursor instances (or custom sprite cursors) for different editor modes (paint, erase, move, pan, selection). Keep cursor objects alive as members and switch via window.setMouseCursor().
- Centralize mouse-inside-window checks in InputController to prevent mouse events from other monitors being processed.

## Audio Integration
- Prepare to implement audio (sound effects and music) integration into the GameEngine+ project.
- Design the music component (CMusic) to be data-only, containing fields such as path, volume, loop, autoplay, and shouldPlay, with a MusicSystem managing playback and analysis.
- Keep audio-reactive effects in MusicVisualizerScene; add UI features to MusicVisualizerScene: include a checkbox to toggle looping for the currently loaded track, and implement a song timer/seek display (playhead) in the music controls.

## Future Work Priorities
- Focus on developing an advanced level editor scene with chunking integration (highest priority - start soon).
- Plan for sound/audio integration (scheduled for tomorrow or later).
- Explore chunk-based physics and collision optimization (high interest).

## Code Style
- Expose needed CShape data members for systems; follow naming and formatting conventions.

## Legacy Notes
- TileMapEditorScene (non-chunked tilemap editor) is legacy/deprecated and does not need chunking. Advanced features like chunking, audio integration, and other engine features are planned for future work. Current focus is exploring chunking capabilities and design patterns.