# Copilot Instructions

## Project Guidelines
- Prefer ECS architecture and logic in systems. 
- Prefer boundary logic in PhysicsSystem and removal of shape-level Includer; prefer avoiding dereferencing pointers after unique_ptr deletion. Use address-based removal in EntityManager::RemoveDeadEntities.
- When removing dead entities, collect dead pointers and remove references from secondary structures by pointer identity using an unordered_set before deleting unique_ptrs; avoid dereferencing raw pointers that may have been freed. Collect dead pointers then remove them from other structures by pointer comparison to avoid use-after-free.
- Defer processing (ProcessPending) to the main update to avoid re-entrancy.
- Render entity text after scene overlays; ensure that entity text is displayed on top of scene overlays for better visibility.
- Plan to create a full sprite system and combine tilemaps with textures; ensure that rendering logic accommodates both sprites and tilemaps effectively.
- Implement interactive audio-reactive shapes by spawning shape entities based on music amplitude/beat via ImGui in TileMapEditorScene. **(Note: Tomorrow, strip music playback and audio-reactive effects out of TileMapEditorScene and keep them only in MusicVisualizerScene.)** Start implementing/experimenting with graphical effects in MusicVisualizerScene.
- Investigate the equalizer issue in MusicVisualizerScene where many bars show identical responses, likely due to MusicSystem providing only 10 spectrum bands. Consider increasing spectrum resolution and enhancing MusicSystem analysis.

## Audio Integration
- Prepare to implement audio (sound effects and music) integration into the GameEngine+ project.
- Design the music component (CMusic) to be data-only, containing fields such as path, volume, loop, autoplay, and shouldPlay, with a MusicSystem managing playback and analysis.
- Keep audio-reactive effects in MusicVisualizerScene; add UI features to MusicVisualizerScene: include a checkbox to toggle looping for the currently loaded track, and implement a song timer/seek display (playhead) in the music controls.

## Code Style
- Expose needed CShape data members for systems; follow naming and formatting conventions.