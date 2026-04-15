#pragma once
#include <optional>
#include <string>

// Native file dialog helpers (Win32 wrapper)
// Owner handle overloads (pass the native window handle, e.g. sf::Window::getSystemHandle())
std::optional<std::string> ShowOpenFileDialog(const std::string& filter = "JSON files\0*.json\0All files\0*.*\0",
											  const std::string& initialDir = "");
std::optional<std::string> ShowSaveFileDialog(const std::string& filter = "JSON files\0*.json\0All files\0*.*\0",
											  const std::string& defaultFileName = "map.json",
											  const std::string& initialDir = "");

// Overloads that accept an owner native window handle (void*). On Windows this should be HWND.
std::optional<std::string>
ShowOpenFileDialogWithOwner(void* ownerHandle, const std::string& filter = "JSON files\0*.json\0All files\0*.*\0",
							const std::string& initialDir = "");
std::optional<std::string>
ShowSaveFileDialogWithOwner(void* ownerHandle, const std::string& filter = "JSON files\0*.json\0All files\0*.*\0",
							const std::string& defaultFileName = "map.json", const std::string& initialDir = "");
