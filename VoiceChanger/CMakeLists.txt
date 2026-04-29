cmake_minimum_required(VERSION 3.16)
project(voicechanger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(VC_API_VERSION "26" CACHE STRING "TS3 plugin API version (23/24/25/26)")

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

add_library(voicechanger SHARED
    src/plugin.cpp
    src/voice_engine.cpp
    src/settings_dialog.cpp
    src/voice_engine.h
    src/settings_dialog.h
    vendor/smb_pitch_shift.h
)

target_include_directories(voicechanger PRIVATE
    ${TS3_SDK_DIR}/include
    src
    vendor
)

target_compile_definitions(voicechanger PRIVATE
    PLUGIN_API_VERSION=${VC_API_VERSION}
    _USE_MATH_DEFINES
)

target_link_libraries(voicechanger PRIVATE Qt5::Core Qt5::Widgets)

if(WIN32)
    set_target_properties(voicechanger PROPERTIES
        PREFIX ""
        OUTPUT_NAME "voicechanger_api${VC_API_VERSION}_win64"
        SUFFIX ".dll"
    )
endif()

if(MSVC)
    target_compile_options(voicechanger PRIVATE /utf-8 /W4)
    target_compile_definitions(voicechanger PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()
