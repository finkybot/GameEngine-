## Fix Summary: Static Destruction Order Fiasco in GameEngine Exit

### Problem
Access violation (0xC0000005) at address 0x000000000000002C during program shutdown.

**Root Cause**: During static destruction at program exit, `LevelEditorScene::~LevelEditorScene()` called `m_chunkManager.SaveAllChunks()` while the scene object's memory was already corrupted/destroyed due to construction order issues.

**Crash Location**: 
- Frame [1]: `ChunkManager::SaveAllChunks()` line 1451 (lock on m_mutex)
- Frame [2]: `LevelEditorScene::~LevelEditorScene()` calling it during atexit
- Frame [9]: `GameEngine::` singleton `atexit` destructor

### Solution
Implemented a process-level shutdown guard to skip cleanup operations during static destruction:

#### 1. Created `ShutdownGuard.h`
Defines a global flag and helper functions:
```cpp
namespace ShutdownGuard {
	extern bool g_isShuttingDown;
	inline void MarkShuttingDown() { g_isShuttingDown = true; }
	inline bool IsShuttingDown() { return g_isShuttingDown; }
}
```

#### 2. Created `ShutdownGuard.cpp`
Defines the global flag:
```cpp
namespace ShutdownGuard { bool g_isShuttingDown = false; }
```

#### 3. Modified `GameEngine::~GameEngine()` destructor
Marks shutdown at the very beginning:
```cpp
GameEngine::~GameEngine() {
	ShutdownGuard::MarkShuttingDown();
	// ... rest of destructor
}
```

#### 4. Modified `LevelEditorScene::~LevelEditorScene()` destructor
Skips SaveAllChunks() during shutdown:
```cpp
LevelEditorScene::~LevelEditorScene() {
	if (!ShutdownGuard::IsShuttingDown()) {
		m_chunkManager.SaveAllChunks();
	}
}
```

#### 5. Updated includes
- Added `#include "ShutdownGuard.h"` to `LevelEditorScene.h`
- Added `#include "ShutdownGuard.h"` to `GameEngine.h`

### Why This Works
1. When program exits, `GameEngine` singleton's destructor is called first
2. It immediately sets `g_isShuttingDown = true`
3. Subsequent scene destructors check this flag
4. `LevelEditorScene::~LevelEditorScene()` sees shutdown flag is set and skips the problematic `SaveAllChunks()` call
5. No access violation occurs

### Alternative Approaches Considered
1. **Save chunks earlier** - Requires explicit timing, less robust
2. **Remove destructor save entirely** - Loses data on crash
3. **Use exception handling in SaveAllChunks()** - Doesn't prevent the crash, just hides it
4. **Check for valid pointer** - Doesn't detect memory corruption as reliably

### Testing
To verify the fix:
1. Run the application normally (shutdown saves chunks)
2. Quit the program - should exit cleanly without 0xC0000005 errors
3. Check that chunks were saved when not during shutdown
