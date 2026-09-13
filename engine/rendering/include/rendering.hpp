#pragma once
#include <siecs.h>
#include <stdint.h>

namespace engine {

struct Color {
    reflected(uint8_t r; uint8_t g; uint8_t b; uint8_t a;)
};

struct Circle {
    reflected(float radius;)
};

struct Rectangle {
    reflected(float width; float height;)
};

struct rendering {
    static void import();
};

} // namespace engine
