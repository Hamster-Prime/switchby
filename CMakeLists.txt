cmake_minimum_required(VERSION 3.15)
project(switchby)

set(CMAKE_CXX_STANDARD 20)

# Dependencies
include(FetchContent)

FetchContent_Declare(
    borealis
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/borealis
)
FetchContent_MakeAvailable(borealis)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# Sources
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Executable
add_executable(switchby ${SOURCES})

FetchContent_Declare(
    libretro-common
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libretro-common
)
FetchContent_MakeAvailable(libretro-common)

# Definitions
add_definitions("-DBOREALIS_RESOURCES='${CMAKE_CURRENT_SOURCE_DIR}/resources'")

# Include dirs
target_include_directories(switchby PRIVATE src)
target_include_directories(switchby PRIVATE ${borealis_SOURCE_DIR}/library/include)
target_include_directories(switchby PRIVATE ${libretro-common_SOURCE_DIR}/include)
target_include_directories(switchby PRIVATE ${nanovg_SOURCE_DIR}/src)

FetchContent_Declare(
    nanovg
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/nanovg
)
FetchContent_MakeAvailable(nanovg)


# Link
find_package(CURL REQUIRED)
# Borealis target is usually just 'borealis'
target_link_libraries(switchby borealis nanovg nanovg_gl3 nlohmann_json::nlohmann_json CURL::libcurl)

# Platform specific configuration
if (${PLATFORM} STREQUAL "SWITCH")
    # Switch build
    # Assuming switch-mpv provides proper CMake or we link it by name
    # Usually switch-mpv is just -lmpv
    target_link_libraries(switchby mpv)

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
    target_link_libraries(switchby ${MPV_LIBRARIES})
    
    # Add compile definition for Desktop if needed
    target_compile_definitions(switchby PRIVATE PLATFORM_DESKTOP)
endif()
