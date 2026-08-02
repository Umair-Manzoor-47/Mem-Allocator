cmake_minimum_required(VERSION 3.20)
project(Mem-Allocator CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_library(alloc INTERFACE)
target_include_directories(alloc INTERFACE ${CMAKE_SOURCE_DIR}/include)


add_executable(main main.cpp)
target_link_libraries(main PRIVATE alloc)


add_subdirectory(tests)