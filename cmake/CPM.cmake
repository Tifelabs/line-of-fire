# Bootstraps CPM.cmake (dependency manager). Tip: set CPM_SOURCE_CACHE in your
# shell (e.g. export CPM_SOURCE_CACHE=$HOME/.cache/CPM) so build dirs share downloads.
set(CPM_DOWNLOAD_VERSION 0.40.2)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT EXISTS "${CPM_DOWNLOAD_LOCATION}")
    message(STATUS "Downloading CPM.cmake v${CPM_DOWNLOAD_VERSION}")
    file(DOWNLOAD
        "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
        "${CPM_DOWNLOAD_LOCATION}")
endif()
include("${CPM_DOWNLOAD_LOCATION}")
