### UI (ImGui)

ImGui is required and always fetched from upstream during configure. The GLFW + OpenGL3 backends are compiled into the app.

### Shaders

The application loads shaders from the `shaders/` folder. You can edit `terrain.vert` and `terrain.frag` and rebuild/re-run.

### VS Code + IntelliSense

If you see include squiggles:

- Run the CMake configure task so `build/compile_commands.json` is generated.
- The workspace is configured to use that file automatically. You can also run “C/C++: Reset IntelliSense Database”.

# OpenGLTerrain (Cross-platform, CMake, GLFW + GLEW + ImGui)

This project renders a simple terrain and cube with OpenGL. It's cross-platform (Windows, Linux, macOS) using GLFW for windowing/input and GLEW for OpenGL loading. ImGui UI is integrated via the GLFW + OpenGL3 backends.

## Requirements

- CMake 3.20+
- A C++23 compiler toolchain
  - Windows: MinGW-w64 or MSVC (Visual Studio)
  - Linux: GCC or Clang with X11 development packages (GLFW will fetch/build deps)
  - macOS: Xcode command line tools
- Optional: clang-tidy, clang-format

All third-party libs (GLFW, GLEW, ImGui) are fetched automatically via CMake's FetchContent; no manual installs required.
Shaders are loaded from source files under `shaders/`.

## Build and run

Windows (MinGW Makefiles):

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -- -j
.\build\OpenGLTerrain.exe
```

Windows (Visual Studio/MSVC):

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\OpenGLTerrain.exe
```

Linux/macOS (Unix Makefiles):

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -- -j
./build/OpenGLTerrain
```

Notes:

- On macOS we request a core profile context; OpenGL is deprecated but still available.
- Shaders are installed alongside the executable; running from the build tree uses `${sourceDir}/shaders`.

## Controls

- Move: W/A/S/D
- Up/Down: Space / Left Shift
- Toggle mouse capture (crosshair mode): ESC

In crosshair mode, the tracer line originates from the crosshair center. With the cursor visible, it points from the mouse to the cube.

## Troubleshooting

- Dependency warnings/noise: The build suppresses warnings from third-party dependencies so only your project warnings are shown.
- Generator mismatch or stale cache: delete the `build/` folder and configure again.
- IntelliSense errors: configure to generate `build/compile_commands.json`, then run “C/C++: Reset IntelliSense Database”.

## Static analysis

If `clang-tidy` is available, it's automatically run during build (disable with `-DENABLE_CLANG_TIDY=OFF`). In Debug builds, some compilers may treat warnings as errors.

## Formatting

Format sources (target is available only if `clang-format` is installed):

```powershell
cmake --build build --target format
```
