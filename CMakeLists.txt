cmake_minimum_required(VERSION 3.20)
project(OpenGLTerrain LANGUAGES CXX)

# C++ standard and tooling
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "")
option(ENABLE_CLANG_TIDY "Run clang-tidy during build if available" ON)

# Dependencies
include(FetchContent)

FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(glew
    GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
    GIT_TAG master)
set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glew)

find_package(OpenGL REQUIRED)

# Suppress warnings from external dependencies during their own compilation
if(MSVC)
    target_compile_options(glfw PRIVATE /W0)
    target_compile_options(libglew_static PRIVATE /W0)
else()
    target_compile_options(glfw PRIVATE -w)
    target_compile_options(libglew_static PRIVATE -w)
endif()

file(GLOB APP_SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Dear ImGui: always fetch from upstream and build with GLFW + OpenGL3 backends
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.0)
FetchContent_MakeAvailable(imgui)
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_BACKENDS_DIR ${IMGUI_DIR}/backends)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_BACKENDS_DIR}/imgui_impl_opengl3.cpp
    ${IMGUI_BACKENDS_DIR}/imgui_impl_glfw.cpp
)

add_definitions(-DUSE_IMGUI)
message(STATUS "ImGui fetched and enabled (GLFW + OpenGL3 backends)")

# Suppress warnings for ImGui sources compiled into our target
if(MSVC)
    set(IMGUI_NO_WARN_FLAGS "/W0 /WX-")
else()
    set(IMGUI_NO_WARN_FLAGS "-w -Wno-error")
endif()
set_source_files_properties(${IMGUI_SOURCES} PROPERTIES COMPILE_FLAGS "${IMGUI_NO_WARN_FLAGS}")

add_executable(${PROJECT_NAME} ${APP_SOURCES} ${IMGUI_SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    SYSTEM PRIVATE
        ${IMGUI_DIR} ${IMGUI_BACKENDS_DIR}
        ${glfw_SOURCE_DIR}/include
        ${glew_SOURCE_DIR}/include)

# Ensure GLFW doesn't try to include legacy GL headers
target_compile_definitions(${PROJECT_NAME} PRIVATE GLFW_INCLUDE_NONE)

# Warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
    endif()
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE glfw OpenGL::GL libglew_static)

# Provide shader directory to the program (absolute path to source shaders, normalized)
set(SHADERS_ABS "${CMAKE_SOURCE_DIR}/shaders")
file(TO_CMAKE_PATH "${SHADERS_ABS}" SHADERS_ABS_POSIX)
target_compile_definitions(${PROJECT_NAME} PRIVATE SHADER_DIR="${SHADERS_ABS_POSIX}")

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
install(DIRECTORY ${CMAKE_SOURCE_DIR}/shaders DESTINATION .)
