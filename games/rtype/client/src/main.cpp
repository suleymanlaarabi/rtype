#include "core.hpp"
#include "gameplay.hpp"
#include "raylib.h"
#include "raylib.hpp"
#include "rendering.hpp"
#include "spatial.hpp"
#include <cstdio>
#include <siecs.h>
#include <siecs_rest.h>
#include <string>

int main() {
    ecs::init({ .target_fps = 60, .worker_threads = 4 });

    ecs::import<engine::core>();
    ecs::import<engine::spatial>();
    ecs::import<engine::rendering>();
    ecs::import<rtype::gameplay>();
    ecs::import<engine::raylib>();
    ecs::import<sirest>();

    ecs::entity::create("rtype::player")
        .set(
            engine::Position(0, 0),
            engine::Velocity(0, 0),
            engine::Rectangle(100, 100),
            engine::Color(255, 0, 0, 255),
            rtype::MoveInput{
                .left = KEY_A,
                .right = KEY_D,
                .up = KEY_W,
                .down = KEY_S,
                .speed = 200,
            },
            rtype::Speed(200)
        )
        .add<rtype::Player, rtype::Gun>();

    ecs::run();
}
