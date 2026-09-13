add_executable(rtype_client
    src/main.cpp
    src/gameplay.cpp
)

target_include_directories(rtype_client
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rtype_client
    PRIVATE
        engine::core
        engine::spatial
        engine::rendering
        engine::raylib
)
