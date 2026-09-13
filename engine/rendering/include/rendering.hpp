#pragma once
#include <cstdint>
#include <siecs.h>

namespace engine {

struct Color {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;
};

struct Circle {
    float radius;
};

struct Rectangle {
    float width;
    float height;
};

struct rendering {
    static void import();
};

} // namespace engine
