include(FetchContent)

find_package(box2d 3.1.1 EXACT QUIET CONFIG)

if(NOT TARGET box2d::box2d)
    FetchContent_Declare(box2d
        URL https://github.com/erincatto/box2d/archive/refs/tags/v3.1.1.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(box2d)
endif()
