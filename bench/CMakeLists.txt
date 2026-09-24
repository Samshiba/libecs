# bench/CMakeLists.txt

add_executable(libecs_bench "Benchmark.cpp")

target_link_libraries(libecs_bench PRIVATE libecs::libecs)

target_compile_options(libecs_bench PRIVATE ${LIBECS_COMPILE_OPTIONS})
