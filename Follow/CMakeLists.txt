cmake_minimum_required(VERSION 3.16)
project(follow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FOLLOW_API_VERSION "26" CACHE STRING "TS3 plugin API version (23/24/25/26)")

# Find ts3sdk: parent or local
if(EXISTS "${CMAKE_SOURCE_DIR}/../ts3sdk/include/plugin_definitions.h")
    set(_TS3_SDK_DEFAULT "${CMAKE_SOURCE_DIR}/../ts3sdk")
else()
    set(_TS3_SDK_DEFAULT "${CMAKE_SOURCE_DIR}/ts3sdk")
endif()
set(TS3_SDK_DIR "${_TS3_SDK_DEFAULT}" CACHE PATH "Path to TeamSpeak 3 Plugin SDK")

if(NOT EXISTS "${TS3_SDK_DIR}/include/plugin_definitions.h")
    message(FATAL_ERROR "TS3 plugin SDK headers not found at ${TS3_SDK_DIR}/include/plugin_definitions.h")
endif()

add_library(follow SHARED src/plugin.cpp)

target_include_directories(follow PRIVATE
    ${TS3_SDK_DIR}/include
    src
)

target_compile_definitions(follow PRIVATE
    PLUGIN_API_VERSION=${FOLLOW_API_VERSION}
)

if(WIN32)
    set_target_properties(follow PROPERTIES
        PREFIX ""
        OUTPUT_NAME "follow_api${FOLLOW_API_VERSION}_win64"
        SUFFIX ".dll"
    )
endif()

if(MSVC)
    target_compile_options(follow PRIVATE /utf-8 /W4)
    target_compile_definitions(follow PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()
