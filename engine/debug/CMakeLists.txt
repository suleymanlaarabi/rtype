add_library(engine_debug STATIC
    src/debug.cpp
)

add_library(engine::debug ALIAS engine_debug)

target_include_directories(engine_debug
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(engine_debug
    PUBLIC
        engine::rendering
)
