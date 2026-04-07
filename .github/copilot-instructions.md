# Copilot Instructions

## Project Guidelines
- Prefer boundary logic in PhysicsSystem and removal of shape-level Includer; prefer avoiding dereferencing pointers after unique_ptr deletion. Use address-based removal in EntityManager::RemoveDeadEntities.
- When removing dead entities, prefer removing references from secondary structures by pointer identity using a set before deleting unique_ptrs; avoid dereferencing pointers that may have been freed. Collect dead pointers then remove them from other structures by pointer comparison (use unordered_set) to avoid use-after-free.
- Avoid calling methods on raw pointers that may have been deleted; when removing entities, collect dead pointers then remove them from other structures by pointer comparison (use unordered_set) to avoid use-after-free.
- Render entity text after scene overlays; ensure that entity text is displayed on top of scene overlays for better visibility.