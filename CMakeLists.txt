cmake_minimum_required(VERSION 3.15)
project(switchby)

set(CMAKE_CXX_STANDARD 20)

# Dependencies
include(FetchContent)

FetchContent_Declare(
    borealis
    GIT_REPOSITORY https://github.com/nativelbs/borealis.git
    GIT_TAG master
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

# Include dirs
target_include_directories(switchby PRIVATE src)

# Link
find_package(CURL REQUIRED)
target_link_libraries(switchby borealis::borealis nlohmann_json::nlohmann_json CURL::libcurl)

# Resources
add_custom_command(TARGET switchby POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources ${CMAKE_BINARY_DIR}/resources)

if (PLATFORM_SWITCH)
    include(${DEVKITPRO}/libnx/switch_rules)
    build_switch_bin(switchby
        TITLE "SwitchBy"
        AUTHOR "Hamster-Prime"
        VERSION "0.1.0"
    )
endif()
