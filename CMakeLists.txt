# CMakeList.txt : Top-level CMake project file

cmake_minimum_required(VERSION 3.28)
project(libecs VERSION 0.1.0 LANGUAGES C CXX)

# Global settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if (PROJECT_IS_TOP_LEVEL)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif ()

# Set up Build Types
if (PROJECT_IS_TOP_LEVEL)
    get_property(multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if (multi)
        set(CMAKE_CONFIGURATION_TYPES "Debug;Profile;Release" CACHE STRING "" FORCE)
    else ()
        if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
            set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
            set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Profile" "Release")
        endif ()
    endif ()
endif ()

set_property(GLOBAL PROPERTY CMAKE_MAP_IMPORTED_CONFIG_PROFILE RelWithDebInfo Release)

# Library
add_library(libecs STATIC)

set(SOURCES
        "src/memory/LinearAllocator.cpp"
        "src/memory/PoolAllocator.cpp"
        "src/core/Registry.cpp"
)

set(HEADERS
        "include/libecs/memory/LinearAllocator.hpp"
        "include/libecs/memory/PoolAllocator.hpp"
        "include/libecs/core/Entity.hpp"
        "include/libecs/core/SparseSet.hpp"
        "include/libecs/core/registry/TypeId.hpp"
        "include/libecs/core/registry/IPool.hpp"
        "include/libecs/core/registry/Registry.hpp"
        "include/libecs/core/view/View.hpp"
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
set(LIBECS_COMPILE_OPTIONS
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
target_compile_options(libecs PRIVATE ${LIBECS_COMPILE_OPTIONS})

# Compile Definitions
target_compile_definitions(libecs PUBLIC
        # Platform
        $<$<PLATFORM_ID:Windows>:LIBECS_PLATFORM_WINDOWS>
        $<$<PLATFORM_ID:Linux>:LIBECS_PLATFORM_LINUX>
        $<$<PLATFORM_ID:Darwin>:LIBECS_PLATFORM_MACOS>

        # Build Type
        $<$<CONFIG:Debug>:LIBECS_DEBUG_BUILD LIBECS_ENABLE_ASSERTS LIBECS_ENABLE_LOGGING>
        $<$<CONFIG:Profile>:LIBECS_PROFILE_BUILD LIBECS_ENABLE_LOGGING>
        $<$<CONFIG:Release>:LIBECS_RELEASE_BUILD>
)

# Testing
option(LIBECS_BUILD_TESTS "Build libecs tests" ${PROJECT_IS_TOP_LEVEL})

if (LIBECS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()