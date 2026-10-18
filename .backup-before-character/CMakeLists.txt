cmake_minimum_required(VERSION 3.25)
project(LineOfFire VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LOF_BUILD_APP "Build the game executable (needs SDL3)" ON)
option(LOF_BUILD_TESTS "Build unit tests" ON)
option(LOF_SANITIZE "Enable address + undefined behaviour sanitizers" OFF)

find_program(LOF_CCACHE ccache)
if(LOF_CCACHE)
    set(CMAKE_C_COMPILER_LAUNCHER "${LOF_CCACHE}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${LOF_CCACHE}")
endif()

include(cmake/CompilerFlags.cmake)
include(cmake/Dependencies.cmake)

# Deterministic core: geometry + simulation. No SDL, no rendering, no floats.
add_library(lof_sim STATIC
    src/geometry/Fixed.cpp
    src/geometry/Intersect.cpp
    src/sim/World.cpp
    src/sim/Step.cpp)
target_include_directories(lof_sim PUBLIC src)
target_compile_features(lof_sim PUBLIC cxx_std_20)
target_link_libraries(lof_sim PUBLIC lof_flags)

# Render-side math (strokes, camera). Floats allowed, still no SDL.
add_library(lof_render_core STATIC
    src/render/Stroke.cpp
    src/render/Camera.cpp)
target_link_libraries(lof_render_core PUBLIC lof_sim)

if(LOF_BUILD_APP)
    add_executable(line_of_fire
        src/app/main.cpp
        src/app/Window.cpp
        src/app/Clock.cpp
        src/render/Renderer.cpp)
    target_link_libraries(line_of_fire PRIVATE lof_render_core ${LOF_SDL_TARGET})
endif()

if(LOF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
