#pragma once
#include "raylib.h"
#include <siecs.h>

namespace rtype {

struct Player {};

struct Gun {};

struct Speed {
    float value;
};

struct MoveInput {
    KeyboardKey left;
    KeyboardKey right;
    KeyboardKey up;
    KeyboardKey down;
    float speed;
};

struct gameplay {
    static void import();
};

} // namespace rtype
