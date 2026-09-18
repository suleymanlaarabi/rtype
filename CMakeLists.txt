cmake_minimum_required(VERSION 3.23)

project(
    rtype
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

add_subdirectory(engine/siecs)
add_subdirectory(engine/core)
add_subdirectory(engine/rendering)
add_subdirectory(engine/debug)
add_subdirectory(engine/network)
add_subdirectory(games/rtype)
