# GameEngine+

`GameEngine+` is a native C++ game engine / sandbox project built with Visual Studio and SFML.

## Requirements

- Windows
- Visual Studio (Desktop development with C++ workload)
- MSVC toolset v145
- Windows SDK 10.0
- SFML 3.1.0

## Included in this repo

- ImGui (GameEngine+/imgui/)
- nlohmann/json (GameEngine+/json.hpp)

## External dependency: SFML 3.1.0

The project is configured to use SFML at:

- Include: `C:\Libraries\SFML-3.1.0\include`
- Lib: `C:\Libraries\SFML-3.1.0\lib`

If your SFML is installed elsewhere, update the project properties.

## Visual Studio setup

1. Open `GameEngine+/GameEngine+.vcxproj` in Visual Studio.
2. Ensure __C/C++ > Additional Include Directories__ contains your SFML include path.
3. Ensure __Linker > Additional Library Directories__ contains your SFML lib path.
4. Confirm the required SFML libs are listed under __Linker > Input > Additional Dependencies__ (debug/release variants).
5. Select `x64` and build.

## Runtime

Provide SFML DLLs next to the executable or add SFML `bin` to your PATH.

## Expected assets

- `assets/fonts/tech.ttf`
- `assets/spawners/default.json`

If missing, the engine will log warnings at startup.

## Building

1. Clone the repository.
2. Install SFML 3.1.0 and update project paths if needed.
3. Open the solution in Visual Studio.
4. Select `x64` and build.

## Troubleshooting

- "SFML headers not found": check Additional Include Directories.
- Linker errors: verify Additional Library Directories and Additional Dependencies.
- Runtime failures: ensure SFML DLLs are available.

---
For contributors: if you want a more detailed CONTRIBUTING.md, open an issue or submit a PR.
