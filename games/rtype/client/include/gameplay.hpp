#pragma once
#include "rendering.hpp"
#include <siecs.h>

namespace rtype {

struct Player {};

struct CameraController {};

struct Gun {
    engine::Key key = engine::Key::Space;
};

struct MoveInput {
    engine::Key left;
    engine::Key right;
    engine::Key up;
    engine::Key down;
    float speed;
};

struct gameplay {
    static void import();
};

} // namespace rtype
