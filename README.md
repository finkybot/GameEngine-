# GameEngine+

`GameEngine+` is a native C++ game engine / sandbox project built with **Visual Studio** and **SFML**.

## Requirements

To build this project locally, install:

- **Windows**
- **Visual Studio 2026** or a recent compatible version
- **Desktop development with C++** workload
- **MSVC toolset v145**
- **Windows SDK 10.0**
- **SFML 3.1.0**

## Included in This Repository

These dependencies are already included in the repo:

- **ImGui** in `GameEngine+/imgui/`
- **nlohmann/json** in `GameEngine+/json.hpp`

## External Dependency Required

### SFML 3.1.0

The Visual Studio project is currently configured to use SFML from:

- Include path: `C:\Libraries\SFML-3.1.0\include`
- Library path: `C:\Libraries\SFML-3.1.0\lib`

If SFML is installed elsewhere, update the project settings.

## Visual Studio Setup

Open `GameEngine+/GameEngine+.vcxproj` and verify:

### Include Directories

__Configuration Properties > C/C++ > General > Additional Include Directories__

- `C:\Libraries\SFML-3.1.0\include`

### Library Directories

__Configuration Properties > Linker > General > Additional Library Directories__

- `C:\Libraries\SFML-3.1.0\lib`

## Required Libraries

### Debug x64

__Configuration Properties > Linker > Input > Additional Dependencies__

- `sfml-graphics-d.lib`
- `sfml-window-d.lib`
- `sfml-system-d.lib`
- `sfml-audio-d.lib`
- `opengl32.lib`

### Release x64

__Configuration Properties > Linker > Input > Additional Dependencies__

- `sfml-graphics.lib`
- `sfml-window.lib`
- `sfml-system.lib`
- `sfml-audio.lib`
- `sfml-network.lib`
- `opengl32.lib`
- `freetype.lib`
- `winmm.lib`
- `gdi32.lib`
- `flac.lib`
- `vorbisenc.lib`
- `vorbisfile.lib`
- `vorbis.lib`
- `ogg.lib`

## Recommended Build Target

Use:

- **Platform:** `x64`
- **Configuration:** `Debug` or `Release`

## Runtime DLLs

The required SFML DLLs must be available at runtime.

Either:

- copy the required SFML DLLs next to the built `.exe`, or
- add the SFML `bin` folder to your system `PATH`

## Important Project Files

- Entry point: `GameEngine+/main.cpp`
- Project file: `GameEngine+/GameEngine+.vcxproj`

## Runtime Assets Expected

The engine expects these files to exist relative to the working directory:

- `assets/fonts/tech.ttf`
- `assets/spawners/default.json`

If `assets/fonts/tech.ttf` is missing, the engine will log a warning and text rendering may fail.

## Build Steps

1. Clone the repository.
2. Install **SFML 3.1.0**.
3. Open the project in Visual Studio.
4. Verify the SFML include and library paths.
5. Select `x64`.
6. Build the project.
7. Ensure the SFML runtime DLLs are available.

## Troubleshooting

### SFML headers not found
Check __Configuration Properties > C/C++ > General > Additional Include Directories__.

### Linker errors for `sfml-*`
Check:

- __Configuration Properties > Linker > General > Additional Library Directories__
- __Configuration Properties > Linker > Input > Additional Dependencies__

### App fails on startup
Make sure the required SFML DLLs are next to the executable or available via `PATH`.

## Files not included in Git
Following files need to be installed in `GameEngine+/x64/Release` These can be found with both the imgui and sfml downloads.

imgui.ini
sfml-audio-3.dll
sfml-audio-d-3.dll
sfml-graphics-3.dll
sfml-graphics-d-3.dll
sfml-network-3.dll
sfml-network-d-3.dll
sfml-system-3.dll
sfml-system-d-3.dll
sfml-window-3.dll
sfml-window-d-3.dll

### Font warning on startup
Make sure `assets/fonts/tech.ttf` exists.

## Summary

To work with this project, a contributor mainly needs:

- Visual Studio with C++ tools
- SFML 3.1.0
- correct SFML include/library paths
- required linker dependencies
- required SFML runtime DLLs
