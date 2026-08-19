#pragma once

namespace ShutdownGuard {
	/// Flag indicating if the application is shutting down
	extern bool g_isShuttingDown;

	/// Mark that the application is shutting down
	inline void MarkShuttingDown() {
		g_isShuttingDown = true;
	}

	/// Check if we're currently shutting down
	inline bool IsShuttingDown() {
		return g_isShuttingDown;
	}
}
