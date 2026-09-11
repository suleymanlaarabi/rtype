add_library(engine_core STATIC
    src/core.cpp
    src/physics.cpp
    src/spatial.cpp
)

add_library(engine::core ALIAS engine_core)

target_include_directories(engine_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(engine_core
    PUBLIC
        box2d::box2d
        flecs::flecs_static
        raylib
)
