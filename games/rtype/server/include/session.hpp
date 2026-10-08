#pragma once

#include <siecs.h>

#include <cstdint>

namespace rtype {

struct Player {
    uint8_t slot = 0;
};

struct Position {
    float x = 0;
    float y = 0;
};

struct Health {
    float current = 0;
    float maximum = 0;
};

// Stored on the connection entity. `player` is the ship this client controls.
struct Session {
    ecs_entity_t player = 0;
    uint8_t slot = 0;
};

struct Lobby {
    static constexpr uint8_t capacity = 4;

    uint8_t occupied = 0;
};

struct session {
    static void import();
};

} // namespace rtype
