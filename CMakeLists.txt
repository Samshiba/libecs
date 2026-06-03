# CMakeList.txt : Top-level CMake project file

cmake_minimum_required(VERSION 3.28)
project(libecs VERSION 1.0.0.0 LANGUAGES C CXX)

# Global settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set up Build Types
get_property(multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (multi)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Profile;Release" CACHE STRING "" FORCE)
else ()
    if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Profile" "Release")
    endif ()
endif ()

set_property(GLOBAL PROPERTY CMAKE_MAP_IMPORTED_CONFIG_PROFILE RelWithDebInfo Release)

# Library
add_library(libecs STATIC)

set(SOURCES
        "src/memory/LinearAllocator.cpp"
)

set(HEADERS
        "include/libecs/memory/LinearAllocator.hpp"
)

target_sources(libecs PRIVATE
        ${SOURCES}
        ${HEADERS}
)

target_include_directories(libecs
        PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"

        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# Compile Options
target_compile_options(libecs PRIVATE
        # --- MSVC Config ---
        $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Debug>:/Od /Zi /RTC1 /MDd>
        $<$<CONFIG:Profile>:/O2 /Zi /MD>
        $<$<CONFIG:Release>:/O2 /MD>
        /W4 /WX /MP /Zc:preprocessor
        >

        # --- GCC/Clang Config ---
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
        $<$<CONFIG:Debug>:-O0 -g>
        $<$<CONFIG:Profile>:-O3 -g>
        $<$<CONFIG:Release>:-O3>
        -Wall -Wextra -Werror -pedantic
        >
)

# Compile Definitions
target_compile_definitions(libecs PUBLIC
        # Platform
        $<$<PLATFORM_ID:Windows>:LIBECS_PLATFORM_WINDOWS>
        $<$<PLATFORM_ID:Linux>:LIBECS_PLATFORM_LINUX>
        $<$<PLATFORM_ID:Darwin>:LIBECS_PLATFORM_MACOS>

        # Build Type
        $<$<OR:$<CONFIG:Release>,$<CONFIG:Profile>>:NDEBUG>
        $<$<CONFIG:Debug>:LIBECS_DEBUG_BUILD LIBECS_ENABLE_ASSERTS LIBECS_ENABLE_LOGGING>
        $<$<CONFIG:Profile>:LIBECS_PROFILE_BUILD LIBECS_ENABLE_LOGGING>
        $<$<CONFIG:Release>:LIBECS_RELEASE_BUILD>

        # SPDLOG
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE>
        $<$<CONFIG:Profile>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO>
        $<$<CONFIG:Release>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF>
)

# Testing
option(LIBECS_BUILD_TESTS "Build libecs tests" ON)

if (LIBECS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()