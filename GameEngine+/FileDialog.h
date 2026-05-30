/////////////////////////////////
// FileDialog.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <optional>
#include <string>
/////////////////////////////////



/////////////////////////////////
// ShowOpenFileDialog and ShowSaveFileDialog functions for displaying native file dialogs to the user. These functions allow the user to select a file to open or specify a file name and location to save, with optional parameters for filtering file types and setting initial directories.
// Native file dialog helpers (Win32 wrapper) Owner handle overloads (pass the native window handle, e.g. sf::Window::getSystemHandle())
std::optional<std::string> ShowOpenFileDialog(const std::string& filter = "JSON files\0*.json\0All files\0*.*\0",
											  const std::string& initialDir = "");
std::optional<std::string> ShowSaveFileDialog(const std::string& filter = "JSON files\0*.json\0All files\0*.*\0",
											  const std::string& defaultFileName = "map.json",
											  const std::string& initialDir = "");
/////////////////////////////////



/////////////////////////////////
// ShowOpenFileDialogWithOwner and ShowSaveFileDialogWithOwner functions that accept an owner native window handle (void*). These functions are similar to the previous ones but allow specifying the owner window for the file dialog, which can help with proper modality and focus behavior 
// on platforms like Windows where the owner handle is typically an HWND. The parameters for filtering file types, setting initial directories, and default file names are also included for convenience.
// Overloads that accept an owner native window handle (void*). On Windows this should be HWND.
std::optional<std::string>ShowOpenFileDialogWithOwner(void* ownerHandle, const std::string& filter = "JSON files\0*.json\0All files\0*.*\0",
							const std::string& initialDir = "");
std::optional<std::string>ShowSaveFileDialogWithOwner(void* ownerHandle, const std::string& filter = "JSON files\0*.json\0All files\0*.*\0",
							const std::string& defaultFileName = "map.json", const std::string& initialDir = "");
/////////////////////////////////