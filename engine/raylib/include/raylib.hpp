#pragma once
#include <siecs.h>
#include <string>

namespace engine {

struct WindowConfig {
    int width{ 800 };
    int height{ 600 };
    std::string title{ "R-Type" };
};

struct raylib {
    static void import();
    static void fini();
};

} // namespace engine
