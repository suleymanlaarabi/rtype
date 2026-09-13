add_library(engine_rendering STATIC
    src/rendering.cpp
)

add_library(engine::rendering ALIAS engine_rendering)

target_include_directories(engine_rendering
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(engine_rendering
    PUBLIC
        engine::siecs
)
