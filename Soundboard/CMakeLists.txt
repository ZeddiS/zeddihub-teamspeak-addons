cmake_minimum_required(VERSION 3.16)
project(soundboard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(SOUNDBOARD_API_VERSION "26" CACHE STRING "TS3 plugin API version (23/24/25/26)")

if(EXISTS "${CMAKE_SOURCE_DIR}/../ts3sdk/include/plugin_definitions.h")
    set(_TS3_SDK_DEFAULT "${CMAKE_SOURCE_DIR}/../ts3sdk")
else()
    set(_TS3_SDK_DEFAULT "${CMAKE_SOURCE_DIR}/ts3sdk")
endif()
set(TS3_SDK_DIR "${_TS3_SDK_DEFAULT}" CACHE PATH "Path to TeamSpeak 3 Plugin SDK")

if(NOT EXISTS "${TS3_SDK_DIR}/include/plugin_definitions.h")
    message(FATAL_ERROR "TS3 plugin SDK headers not found at ${TS3_SDK_DIR}/include/plugin_definitions.h")
endif()

find_package(Qt5 5.12 REQUIRED COMPONENTS Core Widgets)

add_library(soundboard SHARED
    src/plugin.cpp
    src/audio_engine.cpp
    src/audio_engine.h
    src/audio_converter.cpp
    src/audio_converter.h
    src/sound_slot.h
    src/soundboard_dialog.cpp
    src/soundboard_dialog.h
)

target_include_directories(soundboard PRIVATE
    ${TS3_SDK_DIR}/include
    src
)

target_compile_definitions(soundboard PRIVATE
    PLUGIN_API_VERSION=${SOUNDBOARD_API_VERSION}
)

target_link_libraries(soundboard PRIVATE Qt5::Core Qt5::Widgets)

if(WIN32)
    set_target_properties(soundboard PROPERTIES
        PREFIX ""
        OUTPUT_NAME "soundboard_api${SOUNDBOARD_API_VERSION}_win64"
        SUFFIX ".dll"
    )
endif()

if(MSVC)
    target_compile_options(soundboard PRIVATE /utf-8 /W4)
    target_compile_definitions(soundboard PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()
