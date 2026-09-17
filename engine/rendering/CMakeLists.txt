find_package(SDL3 CONFIG REQUIRED)

add_library(engine_rendering STATIC
    src/rendering.cpp
    src/sigpu.c
    src/sigpu_math.c
    src/sigpu_passes.c
    src/sigpu_pipelines.c
    src/sigpu_resources.c
    src/sigpu_visibility.c
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
    PRIVATE
        SDL3::SDL3
)

target_compile_definitions(engine_rendering
    PRIVATE
        SIGPU_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)

if(UNIX)
    target_link_libraries(engine_rendering PRIVATE m)
endif()
