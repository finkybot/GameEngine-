#include "FileDialog.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <vector>
#include <memory>
#include <algorithm>


// Helper functions to convert between UTF-8 and wide strings using Windows API. This is necessary because the Windows file dialog APIs expect wide strings, 
// but we want to use UTF-8 in our application for better cross-platform compatibility and ease of use with std::string.
static std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty()) return {};
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &strTo[0], sizeNeeded, NULL, NULL);
    return strTo;
}


// Convert a UTF-8 encoded std::string to a wide string (std::wstring) using the Windows API. This is necessary for passing strings to Windows functions that expect wide strings, such as the file dialog APIs.
static std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return {};
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    std::wstring w(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], sizeNeeded);
    return w;
}


// Show an open file dialog with the specified filter and initial directory. This function forwards to the owner-aware implementation using the active window as the owner, 
// which helps ensure the dialog appears on top of the game window, especially if it's fullscreen.
std::optional<std::string> ShowOpenFileDialog(const std::string& filter, const std::string& initialDir)
{
    // forward to owner-aware implementation using the active window as owner
    return ShowOpenFileDialogWithOwner(GetActiveWindow(), filter, initialDir);
}


// Show a save file dialog with the specified filter, default file name, and initial directory. This function forwards to the owner-aware implementation using the active window as the owner,
std::optional<std::string> ShowSaveFileDialog(const std::string& filter, const std::string& defaultFileName, const std::string& initialDir)
{
    return ShowSaveFileDialogWithOwner(GetActiveWindow(), filter, defaultFileName, initialDir);
}


// Show an open file dialog with the specified owner window handle, filter, and initial directory. This implementation uses the Win32 API to display a native file dialog, and attempts to ensure 
// it appears on top of the specified owner window (or the active window if no owner is provided) by temporarily minimizing the owner while the dialog is open. The function returns the selected 
// file path as a UTF-8 string, or std::nullopt if the user cancels or an error occurs.
std::optional<std::string> ShowOpenFileDialogWithOwner(void* ownerHandle, const std::string& filter, const std::string& initialDir)
{
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    wchar_t szFile[MAX_PATH] = {0};
    ofn.lStructSize = sizeof(ofn);
    HWND hwnd = static_cast<HWND>(ownerHandle);
    // fallback: try to find the SFML window by title if no owner provided
    if (!hwnd) {
        hwnd = FindWindowW(NULL, L"SFML Game Engine");
    }
    // bring owner to foreground to ensure dialog appears on top
    if (hwnd) {
        SetForegroundWindow(hwnd);
        BringWindowToTop(hwnd);
    }
    ofn.hwndOwner = hwnd;
    std::wstring wfilter = Utf8ToWide(filter);
    ofn.lpstrFilter = wfilter.c_str();
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    if (!initialDir.empty()) {
        std::wstring wdir = Utf8ToWide(initialDir);
        ofn.lpstrInitialDir = wdir.c_str();
    }
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    // If we have an owner window, temporarily minimize it so the dialog appears above (helps with fullscreen windows)
    BOOL result = FALSE;
    if (hwnd) {
        // minimize then disable input while dialog is open
        ShowWindow(hwnd, SW_MINIMIZE);
        EnableWindow(hwnd, FALSE);
    }
    result = GetOpenFileNameW(&ofn);
    if (hwnd) {
        // restore window and re-enable input
        EnableWindow(hwnd, TRUE);
        ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
    }
    if (result) {
        return WideToUtf8(szFile);
    }
    return std::nullopt;
}


// Show a save file dialog with the specified owner window handle, filter, default file name, and initial directory. This implementation uses the Win32 API to display 
// a native file dialog, and attempts to ensure
std::optional<std::string> ShowSaveFileDialogWithOwner(void* ownerHandle, const std::string& filter, const std::string& defaultFileName, const std::string& initialDir)
{
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    wchar_t szFile[MAX_PATH] = {0};
    std::wstring wdef = Utf8ToWide(defaultFileName);
    wcsncpy_s(szFile, wdef.c_str(), _TRUNCATE);
    ofn.lStructSize = sizeof(ofn);
    HWND hwnd = static_cast<HWND>(ownerHandle);
    if (!hwnd) {
        hwnd = FindWindowW(NULL, L"SFML Game Engine");
    }
    if (hwnd) {
        SetForegroundWindow(hwnd);
        BringWindowToTop(hwnd);
    }
    ofn.hwndOwner = hwnd;
    std::wstring wfilter = Utf8ToWide(filter);
    ofn.lpstrFilter = wfilter.c_str();
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    if (!initialDir.empty()) {
        std::wstring wdir = Utf8ToWide(initialDir);
        ofn.lpstrInitialDir = wdir.c_str();
    }
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR; // OFN_OVERWRITEPROMPT will cause the dialog to prompt the user if they select an existing file, which is standard behavior for save dialogs. OFN_NOCHANGEDIR prevents the dialog from changing the current working directory of the application, which can help avoid issues with relative paths after the dialog is closed.

    // If we have an owner window, minimize it so the dialog isn't hidden behind a fullscreen window
    BOOL result = FALSE;
    if (hwnd) {
        ShowWindow(hwnd, SW_MINIMIZE);
        EnableWindow(hwnd, FALSE);
    }
    result = GetSaveFileNameW(&ofn);
    if (hwnd) {
        EnableWindow(hwnd, TRUE);
        ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
    }
    if (result) {
        return WideToUtf8(szFile);
    }
    return std::nullopt;
}


#else
// Stubs for non-Windows platforms
std::optional<std::string> ShowOpenFileDialog(const std::string&, const std::string&) { return std::nullopt; }
std::optional<std::string> ShowSaveFileDialog(const std::string&, const std::string&, const std::string&) { return std::nullopt; }
#endif
