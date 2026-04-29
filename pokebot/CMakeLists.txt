cmake_minimum_required(VERSION 3.16)
project(zeddihub_pokebot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# -----------------------------------------------------------------------------
# TS3 Plugin API version.
#   25 = TS3 client 3.5.6 and similar (max API ~25)
#   26 = TS3 client 3.6.x and newer
# Override with -DPOKEBOT_API_VERSION=25 (default 26).
# -----------------------------------------------------------------------------
set(POKEBOT_API_VERSION "26" CACHE STRING "TS3 plugin API version (25 or 26)")

# -----------------------------------------------------------------------------
# TeamSpeak 3 Plugin SDK
# -----------------------------------------------------------------------------
# Download from https://github.com/TeamSpeak-Systems/ts3client-pluginsdk
# Extract so that ${TS3_SDK_DIR}/include/plugin_definitions.h exists.
# -----------------------------------------------------------------------------
if(EXISTS "${CMAKE_SOURCE_DIR}/../ts3sdk/include/plugin_definitions.h")
    set(_TS3_SDK_DEFAULT "${CMAKE_SOURCE_DIR}/../ts3sdk")
else()
    set(_TS3_SDK_DEFAULT "${CMAKE_SOURCE_DIR}/ts3sdk")
endif()
set(TS3_SDK_DIR "${_TS3_SDK_DEFAULT}" CACHE PATH "Path to TeamSpeak 3 Plugin SDK")

if(NOT EXISTS "${TS3_SDK_DIR}/include/plugin_definitions.h")
    message(FATAL_ERROR
        "TS3 plugin SDK headers not found at ${TS3_SDK_DIR}/include/plugin_definitions.h\n"
        "See ts3sdk/PLACE_SDK_HERE.md for instructions, or pass\n"
        "  -DTS3_SDK_DIR=<path-to-pluginsdk>")
endif()

# -----------------------------------------------------------------------------
# Sources (Win32 native dialog — no Qt dependency)
# -----------------------------------------------------------------------------
set(SOURCES
    src/plugin.cpp
    src/poke_engine.cpp
    src/custom_dialog.cpp
)

set(HEADERS
    src/poke_engine.h
    src/presets.h
    src/custom_dialog.h
)

add_library(zeddihub_pokebot SHARED ${SOURCES} ${HEADERS})

target_include_directories(zeddihub_pokebot PRIVATE
    ${TS3_SDK_DIR}/include
    src
)

target_compile_definitions(zeddihub_pokebot PRIVATE
    PLUGIN_API_VERSION=${POKEBOT_API_VERSION}
)

find_package(Qt5 5.12 REQUIRED COMPONENTS Core Widgets)
target_link_libraries(zeddihub_pokebot PRIVATE Qt5::Core Qt5::Widgets)

# -----------------------------------------------------------------------------
# Output naming — TS3 expects "<name>_win64.dll" on Windows x64
# -----------------------------------------------------------------------------
if(WIN32)
    set_target_properties(zeddihub_pokebot PROPERTIES
        PREFIX ""
        OUTPUT_NAME "zeddihub_pokebot_api${POKEBOT_API_VERSION}_win64"
        SUFFIX ".dll"
    )
elseif(APPLE)
    set_target_properties(zeddihub_pokebot PROPERTIES
        PREFIX ""
        OUTPUT_NAME "zeddihub_pokebot_api${POKEBOT_API_VERSION}_mac"
        SUFFIX ".dylib"
    )
else()
    set_target_properties(zeddihub_pokebot PROPERTIES
        PREFIX ""
        OUTPUT_NAME "zeddihub_pokebot_api${POKEBOT_API_VERSION}_linux_amd64"
        SUFFIX ".so"
    )
endif()

# -----------------------------------------------------------------------------
# MSVC: enable utf-8 source so Czech literals compile correctly
# -----------------------------------------------------------------------------
if(MSVC)
    target_compile_options(zeddihub_pokebot PRIVATE /utf-8 /W4 /permissive-)
    target_compile_definitions(zeddihub_pokebot PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
else()
    target_compile_options(zeddihub_pokebot PRIVATE -Wall -Wextra)
endif()

# -----------------------------------------------------------------------------
# Optional: install rule that drops the DLL into TS3 plugins folder.
# Pass -DTS3_PLUGINS_DIR="C:/Users/<you>/AppData/Roaming/TS3Client/plugins"
# -----------------------------------------------------------------------------
if(DEFINED TS3_PLUGINS_DIR AND EXISTS "${TS3_PLUGINS_DIR}")
    install(TARGETS zeddihub_pokebot RUNTIME DESTINATION "${TS3_PLUGINS_DIR}"
                                     LIBRARY DESTINATION "${TS3_PLUGINS_DIR}")
    message(STATUS "Install target will copy DLL to ${TS3_PLUGINS_DIR}")
endif()
