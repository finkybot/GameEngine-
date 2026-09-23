/////////////////////////////////
// ShutdownGuard.h - Provides a simple mechanism to track whether the application is in the process of shutting down. This can be useful for preventing certain operations or resource allocations during shutdown, ensuring that the application exits cleanly without attempting to perform actions that may no longer be valid.
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
/////////////////////////////////



/////////////////////////////////
//	|	ShutdownGuard namespace - Provides a simple mechanism to track whether the application is in the process of shutting down. This can be useful for preventing certain operations or resource allocations during shutdown, ensuring that the application exits cleanly without attempting to perform actions that may no longer be valid.
//	|_______________________________________________________________________
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
} // namespace ShutdownGuard
	/////////////////////////////////