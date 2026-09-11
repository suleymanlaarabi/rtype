add_executable(rtype
    src/main.cpp
)

target_link_libraries(rtype
    PRIVATE
        engine::core
)

add_test(NAME rtype_hello_world COMMAND rtype)
set_tests_properties(rtype_hello_world PROPERTIES
    PASS_REGULAR_EXPRESSION "Hello, R-Type!"
)
