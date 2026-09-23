# tests/CMakeLists.txt

add_executable(libecs_tests
        "memory/Allocator.test.cpp"
        "core/Entity.test.cpp"
        "core/SparseSet.test.cpp"
        "core/registry/TypeId.test.cpp"
        "core/registry/Registry.test.cpp"
        "core/view/View.test.cpp"
)

# SYSTEM: don't report warnings coming from the vendored doctest header
target_include_directories(libecs_tests
        SYSTEM PRIVATE
        "${PROJECT_SOURCE_DIR}/vendor/doctest"
)

target_link_libraries(libecs_tests PRIVATE libecs::libecs)

target_compile_options(libecs_tests PRIVATE ${LIBECS_COMPILE_OPTIONS})

add_test(NAME libecs_tests COMMAND libecs_tests)
