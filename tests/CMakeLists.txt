# tests/CMakeLists.txt

project(Tests)

add_executable(Tests
        "memory/Allocator.test.cpp"
        "core/Entity.test.cpp"
)

target_include_directories(Tests
        PRIVATE
        "${CMAKE_SOURCE_DIR}/vendor/doctest"
)

target_link_libraries(Tests PRIVATE libecs)

add_test(NAME LibecsTests COMMAND Tests)