cmake_minimum_required(VERSION 3.23)

project(bonfire)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-O3 -ffast-math)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
set(CONAN_CMAKE_SILENT_OUTPUT ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/lib/cmake/utils.cmake)

add_subdirectory(lib/logger)
add_subdirectory(lib/sync)
add_subdirectory(lib/utils)
add_subdirectory(app/videoscaler)
add_subdirectory(app/audiomixer)