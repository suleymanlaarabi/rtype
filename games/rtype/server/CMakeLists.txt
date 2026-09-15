add_executable(rtype_server
    src/main.cpp
)

target_link_libraries(rtype_server
    PRIVATE
        engine::siecs
        engine::network
)

if(UNIX AND NOT APPLE)
    # Export SIECS symbols so runtime-loaded modules can resolve ecs_* calls.
    target_link_options(rtype_server PRIVATE -Wl,--export-dynamic)
endif()

add_test(NAME rtype_server_starts COMMAND rtype_server)
