add_library(engine_spatial STATIC
    src/spatial.cpp
)

add_library(engine::spatial ALIAS engine_spatial)

target_include_directories(engine_spatial
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(engine_spatial
    PUBLIC
        engine::siecs
)
