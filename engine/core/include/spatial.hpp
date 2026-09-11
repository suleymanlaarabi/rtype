#pragma once
#include <flecs.h>
#include <string>

namespace engine {

struct Position {
    float x, y;
};

struct Velocity {
    float x, y;
};

struct spatial {
    explicit spatial(flecs::world &);
};

} // namespace engine
