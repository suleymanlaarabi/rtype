include(FetchContent)

find_package(Threads REQUIRED)

set(SIECS_LOCAL_DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../")

if(EXISTS "${SIECS_LOCAL_DEPS_DIR}/siecs/project.json")
    FetchContent_Declare(siecs SOURCE_DIR "${SIECS_LOCAL_DEPS_DIR}/siecs")
    FetchContent_Declare(sihttp SOURCE_DIR "${SIECS_LOCAL_DEPS_DIR}/sihttp")
    FetchContent_Declare(sicore SOURCE_DIR "${SIECS_LOCAL_DEPS_DIR}/sicore")
    FetchContent_Declare(sireflect SOURCE_DIR "${SIECS_LOCAL_DEPS_DIR}/sireflect")
    FetchContent_Declare(sijson SOURCE_DIR "${SIECS_LOCAL_DEPS_DIR}/sijson")
else()
    FetchContent_Declare(siecs
        URL https://github.com/suleymanlaarabi/siecs/archive/refs/heads/main.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_Declare(sihttp
        URL https://github.com/suleymanlaarabi/sihttp/archive/refs/heads/main.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_Declare(sicore
        URL https://github.com/suleymanlaarabi/sicore/archive/refs/heads/main.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_Declare(sireflect
        URL https://github.com/suleymanlaarabi/sireflect/archive/refs/heads/main.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_Declare(sijson
        URL https://github.com/suleymanlaarabi/sijson/archive/refs/heads/main.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
endif()

FetchContent_MakeAvailable(siecs)
FetchContent_MakeAvailable(sihttp)
FetchContent_MakeAvailable(sicore sireflect sijson)

add_library(engine_siecs STATIC
    ${siecs_SOURCE_DIR}/distr/siecs.c
    ${siecs_SOURCE_DIR}/addons/rest/src/rest.c
    ${siecs_SOURCE_DIR}/addons/rest/src/rest_common.c
    ${siecs_SOURCE_DIR}/addons/rest/src/rest_components.c
    ${siecs_SOURCE_DIR}/addons/rest/src/rest_entities.c
    ${siecs_SOURCE_DIR}/addons/rest/src/rest_reflect.c
    ${siecs_SOURCE_DIR}/addons/rest/src/rest_relations.c
    ${siecs_SOURCE_DIR}/addons/rest/src/rest_scene.c
    ${siecs_SOURCE_DIR}/addons/rest/src/rest_modules.c
    ${siecs_SOURCE_DIR}/addons/rest/src/rest_schema.c
    ${sihttp_SOURCE_DIR}/src/siformat.c
    ${sihttp_SOURCE_DIR}/src/sihttp.c
    ${sihttp_SOURCE_DIR}/src/sihttp_buffer.c
    ${sihttp_SOURCE_DIR}/src/sihttp_request.c
    ${sihttp_SOURCE_DIR}/src/sihttp_response.c
    ${sihttp_SOURCE_DIR}/src/sihttp_route.c
)

add_library(engine::siecs ALIAS engine_siecs)

target_include_directories(engine_siecs
    PUBLIC
        $<BUILD_INTERFACE:${siecs_SOURCE_DIR}/distr>
        $<BUILD_INTERFACE:${siecs_SOURCE_DIR}/addons/rest/include>
        $<BUILD_INTERFACE:${sihttp_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${sicore_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${sireflect_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${sijson_SOURCE_DIR}/include>
)

target_link_libraries(engine_siecs
    PUBLIC
        Threads::Threads
        ${CMAKE_DL_LIBS}
)

if(MSVC)
    target_compile_options(engine_siecs PRIVATE /experimental:c11atomics)
elseif(UNIX)
    target_compile_definitions(engine_siecs PRIVATE _POSIX_C_SOURCE=200809L)
endif()
