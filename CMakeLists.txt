cmake_minimum_required(VERSION 3.15)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
project(Demogobbler VERSION 1.0.0)

set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")

option(DEMOGOBBLER_TEST "Testing enabled" ON)
option(DEMOGOBBLER_BENCH "Benching enabled" ON)

if(DEMOGOBBLER_TEST)
enable_testing()
add_subdirectory(thirdparty/googletest)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(test)
endif()

if(DEMOGOBBLER_BENCH)
option(BENCHMARK_ENABLE_TESTING "Enable testing of the benchmark library." OFF)
add_subdirectory(thirdparty/benchmark)
add_subdirectory(bench)
endif()

add_subdirectory(src)
add_subdirectory(tools)
