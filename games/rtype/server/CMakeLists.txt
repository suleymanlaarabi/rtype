add_executable(rtype_server
    src/main.cpp
)

target_link_libraries(rtype_server
    PRIVATE
        engine::siecs
)

add_test(NAME rtype_server_starts COMMAND rtype_server)
