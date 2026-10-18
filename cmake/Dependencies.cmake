include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

if(LOF_BUILD_APP)
    CPMAddPackage(
        NAME SDL3
        GITHUB_REPOSITORY libsdl-org/SDL
        GIT_TAG release-3.2.0
        OPTIONS
            "SDL_SHARED OFF"
            "SDL_STATIC ON"
            "SDL_TESTS OFF"
            "SDL_TEST_LIBRARY OFF")
    if(TARGET SDL3::SDL3)
        set(LOF_SDL_TARGET SDL3::SDL3)
    else()
        set(LOF_SDL_TARGET SDL3::SDL3-static)
    endif()
endif()

if(LOF_BUILD_TESTS)
    # Header-only: download the source and skip doctest's own (old) CMakeLists.
    CPMAddPackage(
        NAME doctest
        GITHUB_REPOSITORY doctest/doctest
        GIT_TAG v2.4.11
        DOWNLOAD_ONLY YES)
    add_library(doctest INTERFACE)
    target_include_directories(doctest SYSTEM INTERFACE "${doctest_SOURCE_DIR}")
    add_library(doctest::doctest ALIAS doctest)
endif()

# Added later, when the milestone needs them (pin exact tags):
#   Dear ImGui (debug UI / level editor), ENet (transport), miniaudio (foley)
