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

if ("${PLATFORM}" STREQUAL "SWITCH")
    set(PLATFORM_SWITCH ON CACHE BOOL "" FORCE)
else()
    set(PLATFORM_DESKTOP ON CACHE BOOL "" FORCE)
endif()

# --- Dependencies ---
include(FetchContent)

# Declare all dependencies first
FetchContent_Declare(
    borealis
    GIT_REPOSITORY https://github.com/xfangfang/borealis.git
    GIT_TAG wiliwili
)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_Declare(
    libretro-common
    GIT_REPOSITORY https://github.com/libretro/libretro-common.git
    GIT_TAG master
)
FetchContent_Declare(
    nanovg
    GIT_REPOSITORY https://github.com/memononen/nanovg.git
    GIT_TAG master
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
add_definitions("-DBRLS_RESOURCES='${CMAKE_CURRENT_SOURCE_DIR}/resources'")

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
    nlohmann_json
    CURL::libcurl
)

# --- Platform Specific Configuration ---
if ("${PLATFORM}" STREQUAL "SWITCH")
    # Switch build; libmpv needs Lua (from LUA_ROOT in CI, or pkg-config)
    set(LUA_LINK_LIBS "")
    if(LUA_ROOT)
        target_include_directories(switchby PRIVATE ${LUA_ROOT}/include)
        target_link_directories(switchby PRIVATE ${LUA_ROOT}/lib)
        list(APPEND LUA_LINK_LIBS lua)
    else()
        find_package(PkgConfig REQUIRED)
        foreach(LUA_PC lua54 lua5.4 lua)
            pkg_check_modules(LUA ${LUA_PC} QUIET)
            if(LUA_FOUND)
                target_include_directories(switchby PRIVATE ${LUA_INCLUDE_DIRS})
                list(APPEND LUA_LINK_LIBS ${LUA_LIBRARIES})
                break()
            endif()
        endforeach()
    endif()
    target_link_libraries(switchby PRIVATE 
        mpv
        avfilter
        avformat
        avcodec
        postproc
        swresample
        swscale
        avutil
        ass
        placebo
        archive
        curl
        dav1d
        mbedtls
        mbedx509
        mbedcrypto
        freetype
        harfbuzz
        fribidi
        expat
        bz2
        lzma
        zstd
        lz4
        png
        jpeg
        z
        SDL2
        EGL
        GLESv2
        glapi
        drm_nouveau
        nx
        ${LUA_LINK_LIBS}
    )

    nx_create_nro(switchby
        ROMFS "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    )
else()
    # Desktop build
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MPV REQUIRED mpv)

    target_include_directories(switchby PRIVATE ${MPV_INCLUDE_DIRS})
    target_link_libraries(switchby PRIVATE ${MPV_LIBRARIES})

    target_compile_definitions(switchby PRIVATE PLATFORM_DESKTOP)
endif()
