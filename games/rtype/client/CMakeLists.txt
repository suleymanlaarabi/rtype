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
        engine::rendering
        engine::network
)

if(UNIX AND NOT APPLE)
    # Export SIECS symbols for runtime-loaded C/C++ modules.
    target_link_options(rtype_client PRIVATE -Wl,--export-dynamic)
endif()
