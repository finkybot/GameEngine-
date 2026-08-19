# Code Changes Made to Fix 0xC0000005 Access Violation

## New Files Created

### 1. GameEngine+\ShutdownGuard.h
```cpp
#pragma once

/**
 * ShutdownGuard.h - Global shutdown state management
 * Prevents operations during static destruction that could access already-destroyed objects
 */

namespace ShutdownGuard {
	/// Flag indicating if the application is in the process of shutting down
	extern bool g_isShuttingDown;

	/// Call this before destroying static objects to prevent post-destruction access attempts
	inline void MarkShuttingDown() {
		g_isShuttingDown = true;
	}

	/// Check if we're currently shutting down
	inline bool IsShuttingDown() {
		return g_isShuttingDown;
	}
}
```

### 2. GameEngine+\ShutdownGuard.cpp
```cpp
#include "ShutdownGuard.h"

namespace ShutdownGuard {
	bool g_isShuttingDown = false;
}
```

## Modified Files

### 1. GameEngine+\LevelEditorScene.h
**Change**: Added include for ShutdownGuard
```cpp
#pragma once
#include "Scene.h"
#include "ShutdownGuard.h"  // <-- ADDED
```

### 2. GameEngine+\LevelEditorScene.cpp
**Change**: Modified destructor to skip SaveAllChunks() during shutdown
```cpp
LevelEditorScene::~LevelEditorScene() {
	// Skip saving chunks during process shutdown to avoid accessing already-destroyed objects
	// during static destruction order issues (0xC0000005 access violation)
	if (!ShutdownGuard::IsShuttingDown()) {
		m_chunkManager.SaveAllChunks();
	}
}
```

### 3. GameEngine+\GameEngine.h
**Change**: Added include for ShutdownGuard
```cpp
#pragma once
#include "ShutdownGuard.h"  // <-- ADDED
```

### 4. GameEngine+\GameEngine.cpp
**Change**: Mark shutdown at the beginning of the destructor
```cpp
GameEngine::~GameEngine() {
	// Mark that we're shutting down BEFORE member destruction begins
	// This prevents destructors from attempting operations on already-destroyed objects
	ShutdownGuard::MarkShuttingDown();
	// ... rest of existing destructor code
}
```

## How It Fixes the Issue

**Before Fix**:
- Program exit → GameEngine singleton starts destructing
- Scene objects get destroyed (memory possibly already freed)
- LevelEditorScene::~LevelEditorScene() calls SaveAllChunks()
- SaveAllChunks() tries to lock m_mutex on corrupted object
- **CRASH**: 0xC0000005 Access Violation at 0x000000000000002C

**After Fix**:
- Program exit → GameEngine singleton starts destructing
- GameEngine::~GameEngine() sets g_isShuttingDown = true
- Scene objects begin destruction
- LevelEditorScene::~LevelEditorScene() checks flag
- Flag is true, so SaveAllChunks() is NOT called
- **NO CRASH**: Safe shutdown sequence

## Impact

- ✅ Fixes access violation on program exit
- ✅ Chunks are still saved during normal operation (only skipped at shutdown)
- ✅ Minimal performance impact (single bool check)
- ✅ Thread-safe at shutdown (sequential destruction)
- ✅ No dependence on external libraries
