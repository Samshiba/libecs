# CMakeList.txt : Top-level CMake project file

cmake_minimum_required(VERSION 3.28)

set(LIBECS_VERSION 0.1.0)

project(libecs
        VERSION ${LIBECS_VERSION}
        DESCRIPTION "A small sparse-set Entity Component System in C++20"
        LANGUAGES CXX)

# Options as main project
option(LIBECS_BUILD_TESTS "Build libecs tests" ${PROJECT_IS_TOP_LEVEL})
option(LIBECS_BUILD_BENCHMARKS "Build libecs benchmarks" ${PROJECT_IS_TOP_LEVEL})
option(LIBECS_INSTALL "Generate libecs install rules" ${PROJECT_IS_TOP_LEVEL})
option(LIBECS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ${PROJECT_IS_TOP_LEVEL})

# Top-level only settings
if (PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    # Build Types
    get_property(multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if (multi)
        set(CMAKE_CONFIGURATION_TYPES "Debug;Profile;Release" CACHE STRING "" FORCE)
    elseif (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Profile" "Release")
    endif ()
endif ()

# Library
add_library(libecs STATIC)
add_library(libecs::libecs ALIAS libecs)

set_target_properties(libecs PROPERTIES
        OUTPUT_NAME ecs
        EXPORT_NAME libecs
        DEBUG_POSTFIX d
)

# need C++20 to compile the libecs headers
target_compile_features(libecs PUBLIC cxx_std_20)

target_sources(libecs
        PRIVATE
        "src/memory/VirtualMemory.hpp"
        "src/memory/VirtualMemory.cpp"
        "src/memory/LinearAllocator.cpp"
        "src/memory/PoolAllocator.cpp"
        "src/core/Registry.cpp"

        # FILE_SET: sets the public include dir and the headers to install
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "include"
        FILES
        "include/libecs/memory/LinearAllocator.hpp"
        "include/libecs/memory/PoolAllocator.hpp"
        "include/libecs/core/Entity.hpp"
        "include/libecs/core/SparseSet.hpp"
        "include/libecs/core/registry/TypeId.hpp"
        "include/libecs/core/registry/IPool.hpp"
        "include/libecs/core/registry/Registry.hpp"
        "include/libecs/core/view/View.hpp"
        "include/libecs/core/view/ViewIterator.hxx"
)

target_include_directories(libecs PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Compile Options (applied PRIVATE to libecs)
set(LIBECS_COMPILE_OPTIONS
        # --- MSVC Config ---
        $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Profile>:/O2 /Zi>
        /W4 /permissive- /Zc:preprocessor
        $<$<BOOL:${LIBECS_WARNINGS_AS_ERRORS}>:/WX>
        >

        # --- GCC/Clang Config ---
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
        $<$<CONFIG:Profile>:-O3 -g>
        -Wall -Wextra -pedantic
        $<$<BOOL:${LIBECS_WARNINGS_AS_ERRORS}>:-Werror>
        >
)
target_compile_options(libecs PRIVATE ${LIBECS_COMPILE_OPTIONS})

# Testing
if (LIBECS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

# Benchmarks
if (LIBECS_BUILD_BENCHMARKS)
    add_subdirectory(bench)
endif ()

# Install / export: enables find_package(libecs) after `cmake --install`
if (LIBECS_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(TARGETS libecs
            EXPORT libecsTargets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT libecsTargets
            NAMESPACE libecs::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libecs
    )

    configure_package_config_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/libecsConfig.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/libecsConfig.cmake"
            INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libecs
    )

    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/libecsConfigVersion.cmake"
            COMPATIBILITY SameMinorVersion
    )

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/libecsConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/libecsConfigVersion.cmake"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libecs
    )
endif ()
