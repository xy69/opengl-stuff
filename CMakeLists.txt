cmake_minimum_required(VERSION 3.20)

# Project setup
project(OpenGLTerrain LANGUAGES CXX)

# Use C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for clang-tidy/clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(ENABLE_IMGUI "Build with Dear ImGui UI" ON)
option(ENABLE_CLANG_TIDY "Run clang-tidy during build if available" ON)

# Gather sources (keep vendor code explicit; avoid formatting/tidy on third_party by default)
file(GLOB APP_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
set(IMGUI_BACKENDS_DIR ${IMGUI_DIR}/backends)

if(ENABLE_IMGUI)
    list(APPEND APP_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_BACKENDS_DIR}/imgui_impl_win32.cpp
        ${IMGUI_BACKENDS_DIR}/imgui_impl_opengl3.cpp
    )
endif()

# Executable: WIN32 to use WinMain and avoid console window
add_executable(${PROJECT_NAME} WIN32 ${APP_SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${IMGUI_DIR}
        ${IMGUI_BACKENDS_DIR}
)

if(ENABLE_IMGUI)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_IMGUI)
endif()

# Warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
    endif()
endif()

# Link system libraries for Win32 + OpenGL
if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            opengl32
            gdi32
            user32
            shell32
            imm32
            dwmapi
            ole32
    )
endif()

# Clang-Tidy integration (optional)
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-18 clang-tidy-17 clang-tidy-16)
    if(CLANG_TIDY_EXE)
        message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-p=${CMAKE_BINARY_DIR}"
        )
    else()
        message(STATUS "clang-tidy not found; skipping static analysis")
    endif()
endif()

# Add a 'format' target to run clang-format on our sources/headers
find_program(CLANG_FORMAT_EXE NAMES clang-format clang-format-18 clang-format-17 clang-format-16)
if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE FORMAT_FILES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i ${FORMAT_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running clang-format on project sources"
    )
endif()

# Install (optional)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
