include(FetchContent)

find_package(Threads REQUIRED)

set(SI_DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../" CACHE PATH "Path to local SI dependencies")

set(SI_DEPS siecs sihttp sicore sireflect sijson)
foreach(dep ${SI_DEPS})
    if(EXISTS "${SI_DEPS_DIR}/${dep}")
        FetchContent_Declare(${dep} SOURCE_DIR "${SI_DEPS_DIR}/${dep}")
    else()
        FetchContent_Declare(${dep}
            URL "https://github.com/suleymanlaarabi/${dep}/archive/refs/heads/main.tar.gz"
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
    endif()
endforeach()

FetchContent_MakeAvailable(${SI_DEPS})

file(GLOB SIECS_REST_SRCS CONFIGURE_DEPENDS "${siecs_SOURCE_DIR}/addons/rest/src/*.c")
file(GLOB SIECS_SPATIAL_SRCS CONFIGURE_DEPENDS "${siecs_SOURCE_DIR}/addons/spatial/src/*.c")
file(GLOB SIHTTP_SRCS CONFIGURE_DEPENDS "${sihttp_SOURCE_DIR}/src/*.c")

add_library(engine_siecs STATIC
    ${siecs_SOURCE_DIR}/distr/siecs.c
    ${SIECS_REST_SRCS}
    ${SIECS_SPATIAL_SRCS}
    ${SIHTTP_SRCS}
)

add_library(engine::siecs ALIAS engine_siecs)

target_include_directories(engine_siecs
    PUBLIC
        $<BUILD_INTERFACE:${siecs_SOURCE_DIR}/distr>
        $<BUILD_INTERFACE:${siecs_SOURCE_DIR}/addons/rest/include>
        $<BUILD_INTERFACE:${siecs_SOURCE_DIR}/addons/spatial/include>
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

if(UNIX)
    target_link_libraries(engine_siecs PUBLIC m)
endif()

if(MSVC)
    target_compile_options(engine_siecs PRIVATE /experimental:c11atomics)
elseif(UNIX)
    target_compile_definitions(engine_siecs PRIVATE _POSIX_C_SOURCE=200809L)
endif()
