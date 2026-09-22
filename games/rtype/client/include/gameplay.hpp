#pragma once
#include <siecs.h>
#include <sigpu.h>

namespace rtype {

struct Player {};

struct CameraController {};

struct Gun {
    Key key = Key::Space;
};

struct MoveInput {
    reflected(uint8_t left; uint8_t right; uint8_t up; uint8_t down; float speed;);
};

struct gameplay {
    static void import();
};

} // namespace rtype
