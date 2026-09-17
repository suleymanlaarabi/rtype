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
    using EngineKey = engine::Key;

    reflected(EngineKey left; EngineKey right; EngineKey up; EngineKey down; float speed;);
};

struct gameplay {
    static void import();
};

} // namespace rtype
