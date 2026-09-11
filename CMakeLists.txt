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

add_subdirectory(engine/flecs)
add_subdirectory(engine/raylib)
add_subdirectory(engine/box2d)
add_subdirectory(engine/core)
add_subdirectory(games/rtype)

# Plus tard :
# add_subdirectory(engine/renderer)
# add_subdirectory(engine/audio)
# add_subdirectory(games/game3d)
