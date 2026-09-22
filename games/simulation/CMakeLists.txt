add_executable(simulation
    src/main.cpp
    src/simulation.cpp
    src/scene.cpp
    src/prefabs.cpp
    src/spatial_grid.cpp
    src/targeting.cpp
    src/steering.cpp
    src/weapons.cpp
    src/projectiles.cpp
    src/damage.cpp
    src/debug.cpp
)

target_include_directories(simulation
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(simulation
    PRIVATE
        engine::core
        engine::debug
)

if(UNIX AND NOT APPLE)
    target_link_options(simulation PRIVATE -Wl,--export-dynamic)
endif()
