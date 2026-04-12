# Copilot Instructions

## Project Guidelines
- Prefer boundary logic in PhysicsSystem and removal of shape-level Includer; prefer avoiding dereferencing pointers after unique_ptr deletion. Use address-based removal in EntityManager::RemoveDeadEntities.
- When removing dead entities, prefer removing references from secondary structures by pointer identity using a set before deleting unique_ptrs; avoid dereferencing pointers that may have been freed. Collect dead pointers then remove them from other structures by pointer comparison (use unordered_set) to avoid use-after-free.
- Avoid calling methods on raw pointers that may have been deleted; when removing entities, collect dead pointers then remove them from other structures by pointer comparison (use unordered_set) to avoid use-after-free.
- Render entity text after scene overlays; ensure that entity text is displayed on top of scene overlays for better visibility.
- Plan to create a full sprite system and combine tilemaps with textures; ensure that rendering logic accommodates both sprites and tilemaps effectively.
- Implement interactive audio-reactive shapes by spawning shape entities based on music amplitude/beat via ImGui in TileMapEditorScene. **(Note: Tomorrow, strip music playback and audio-reactive effects out of TileMapEditorScene and keep them only in MusicVisualizerScene.)** Start implementing/experimenting with graphical effects in MusicVisualizerScene.

## Audio Integration
- Prepare to implement audio (sound effects and music) integration into the GameEngine+ project.
- Design the music component to be data-only, with a MusicSystem managing playback. The Music component should contain only data fields such as path, volume, loop, autoplay, and shouldPlay.
- Add UI features to MusicVisualizerScene: include a checkbox to toggle looping for the currently loaded track, and implement a song timer/seek display (playhead) in the music controls.