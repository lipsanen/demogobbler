cmake_minimum_required(VERSION 3.15)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
project(Demogobbler VERSION 1.0.0)

option(DEMOGOBBLER_TEST "Testing enabled" ON)

if(DEMOGOBBLER_TEST)
enable_testing()
add_subdirectory(thirdparty/googletest)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(test)
endif()

add_subdirectory(src)
add_subdirectory(tools)
