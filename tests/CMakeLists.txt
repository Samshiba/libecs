# tests/CMakeLists.txt

project(Tests)

add_executable(Tests
        "memory/Allocator.test.cpp"
        "core/Entity.test.cpp"
        "core/SparseSet.test.cpp"
        "core/registry/TypeId.test.cpp"
        "core/registry/Registry.test.cpp"
        "core/view/View.test.cpp"
)

target_include_directories(Tests
        PRIVATE
        "${PROJECT_SOURCE_DIR}/vendor/doctest"
)

target_link_libraries(Tests PRIVATE libecs)

target_compile_options(Tests PRIVATE ${LIBECS_COMPILE_OPTIONS})

add_test(NAME LibecsTests COMMAND Tests)
