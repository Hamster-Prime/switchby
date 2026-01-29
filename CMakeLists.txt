cmake_minimum_required(VERSION 3.15)
project(switchby)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set a default build type if none was specified (e.g., Release)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
  message(STATUS "Setting build type to 'Release' as default.")
endif()

# --- Dependencies ---
# Using FetchContent to ensure reproducible builds by locking dependency versions.
# This replaces the old git submodules.
include(FetchContent)

# Declare all dependencies first
FetchContent_Declare(
    borealis
    GIT_REPOSITORY https://github.com/nativelbs/borealis.git
    GIT_TAG main # For stability, it's best to lock this to a specific commit hash or release tag
)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_Declare(
    libretro-common
    GIT_REPOSITORY https://github.com/libretro/libretro-common.git
    GIT_TAG v1.16.0 # Using a recent stable tag
)
FetchContent_Declare(
    nanovg
    GIT_REPOSITORY https://github.com/memononen/nanovg.git
    GIT_TAG master # Best to lock to a specific commit hash
)

# Make all dependencies available for use
FetchContent_MakeAvailable(borealis json libretro-common nanovg)

# --- Sources ---
# Explicitly list source files for a more robust build.
# Please add all your .cpp files here.
set(SOURCES
    src/main.cpp
    src/api/emby_client.cpp
    src/activity/detail_activity.cpp
    src/activity/player_activity.cpp
)

# --- Executable ---
add_executable(switchby ${SOURCES})

# --- Definitions & Includes ---
add_definitions("-DBOREALIS_RESOURCES='${CMAKE_CURRENT_SOURCE_DIR}/resources'")

target_include_directories(switchby PRIVATE
    src
    ${borealis_SOURCE_DIR}/library/include
    ${libretro-common_SOURCE_DIR}/include
    ${nanovg_SOURCE_DIR}/src
)

# --- Compiler Optimizations ---
# Add performance optimizations for Release builds, crucial for Switch.
target_compile_options(switchby PRIVATE
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
    $<$<CONFIG:Debug>:-g>
)

# --- Linking ---
find_package(CURL REQUIRED)
target_link_libraries(switchby PRIVATE
    borealis
    nanovg
    nlohmann_json::nlohmann_json
    CURL::libcurl
)

# --- Platform Specific Configuration ---
if (${PLATFORM} STREQUAL "SWITCH")
    # Switch build
    target_link_libraries(switchby PRIVATE mpv) # Use PRIVATE for better encapsulation

    include(${DEVKITPRO}/libnx/switch_rules)
    build_switch_bin(switchby
        TITLE "SwitchBy"
        AUTHOR "Hamster-Prime"
        VERSION "0.1.0"
    )
else()
    # Desktop build
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MPV REQUIRED mpv)

    target_include_directories(switchby PRIVATE ${MPV_INCLUDE_DIRS})
    target_link_libraries(switchby PRIVATE ${MPV_LIBRARIES})

    target_compile_definitions(switchby PRIVATE PLATFORM_DESKTOP)
endif()
