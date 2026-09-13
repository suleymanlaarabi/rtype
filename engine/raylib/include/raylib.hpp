#pragma once
#include <siecs.h>

namespace engine {

struct WindowConfig {
    int width;
    int height;
    const char *title;
};

struct raylib {
    static void import();
    static void fini();
};

} // namespace engine
