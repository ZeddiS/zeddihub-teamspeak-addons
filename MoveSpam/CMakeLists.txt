cmake_minimum_required(VERSION 3.16)
project(movespam LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(MOVESPAM_API_VERSION "26" CACHE STRING "TS3 plugin API version (23/24/25/26)")

# Find ts3sdk
if(EXISTS "${CMAKE_SOURCE_DIR}/../ts3sdk/include/plugin_definitions.h")
    set(_TS3_SDK_DEFAULT "${CMAKE_SOURCE_DIR}/../ts3sdk")
else()
    set(_TS3_SDK_DEFAULT "${CMAKE_SOURCE_DIR}/ts3sdk")
endif()
set(TS3_SDK_DIR "${_TS3_SDK_DEFAULT}" CACHE PATH "Path to TeamSpeak 3 Plugin SDK")

if(NOT EXISTS "${TS3_SDK_DIR}/include/plugin_definitions.h")
    message(FATAL_ERROR "TS3 plugin SDK headers not found at ${TS3_SDK_DIR}/include/plugin_definitions.h")
endif()

# Qt 5.15.2 for the custom dialog
find_package(Qt5 5.15 REQUIRED COMPONENTS Core Widgets)

add_library(movespam SHARED
    src/plugin.cpp
    src/move_engine.cpp
    src/custom_dialog.cpp
    src/move_engine.h
    src/custom_dialog.h
)

target_include_directories(movespam PRIVATE
    ${TS3_SDK_DIR}/include
    src
)

target_compile_definitions(movespam PRIVATE
    PLUGIN_API_VERSION=${MOVESPAM_API_VERSION}
)

target_link_libraries(movespam PRIVATE Qt5::Core Qt5::Widgets)

if(WIN32)
    set_target_properties(movespam PROPERTIES
        PREFIX ""
        OUTPUT_NAME "movespam_api${MOVESPAM_API_VERSION}_win64"
        SUFFIX ".dll"
    )
endif()

if(MSVC)
    target_compile_options(movespam PRIVATE /utf-8 /W4)
    target_compile_definitions(movespam PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()
