# OpenGLTerrain (CMake + clang-tools)

This project is configured for CMake, C++23, and optional clang-tidy/clang-format.

## Requirements

- CMake 3.20+
- MinGW-w64 toolchain (g++, gcc, gdb in PATH)
- Optional: clang-tidy, clang-format

## Build (MinGW-w64)

```powershell
# Configure
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release -- -j

# Run (binary is in build/)
./build/OpenGLTerrain.exe
```

## Static analysis

If `clang-tidy` is available, it's automatically run during build (can be disabled with `-DENABLE_CLANG_TIDY=OFF`).

In Debug builds, compiler warnings are treated as errors for project sources to catch issues early.

## Formatting

Format sources (target is available only if `clang-format` is installed):

```powershell
cmake --build build --target format
```

## ImGui

ImGui is included under `third_party/imgui`. You can disable compiling ImGui by passing `-DENABLE_IMGUI=OFF` to `cmake`.
