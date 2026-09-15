add_library(engine_network STATIC
    src/network.cpp
)

add_library(engine::network ALIAS engine_network)

target_include_directories(engine_network
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(engine_network
    PUBLIC
        engine::siecs
)
