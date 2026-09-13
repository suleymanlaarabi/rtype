#pragma once
#include "raylib.h"
#include <siecs.h>

namespace rtype {

struct Player {};

struct Gun {};

struct Speed {
    reflected(float value;)
};

struct MoveInput {
    reflected(int left; int right; int up; int down; float speed;)
};

struct gameplay {
    static void import();
};

} // namespace rtype
