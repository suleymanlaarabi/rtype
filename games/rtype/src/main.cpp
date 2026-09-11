#include <core.hpp>
#include <cstddef>
#include <flecs.h>
#include <iostream>
#include <raylib.h>
#include <spatial.hpp>

struct CharacterController {
    KeyboardKey left;
    KeyboardKey right;
    KeyboardKey top;
    KeyboardKey bottom;
};

struct Gun {};

struct Player {};

struct Speed {
    float value;
};

int main() {
    flecs::world world;
    world.import<engine::module>();

    world.system<const CharacterController, const Speed, engine::Velocity>().each(
        [](const CharacterController &controller, const Speed &speed, engine::Velocity &vel) {
            float dx = 0;
            float dy = 0;
            if (IsKeyDown(controller.left)) {
                dx = -speed.value;
            }
            if (IsKeyDown(controller.right)) {
                dx = speed.value;
            }
            if (IsKeyDown(controller.top)) {
                dy = -speed.value;
            }
            if (IsKeyDown(controller.bottom)) {
                dy = speed.value;
            }
            vel.x = dx;
            vel.y = dy;
        }
    );

    flecs::entity player = world.entity()
                               .add<engine::Position>()
                               .add<engine::Velocity>()
                               .set(engine::Rectangle{ 100, 100 })
                               .set(Color{ 255, 0, 0, 255 })
                               .set(CharacterController{ KEY_A, KEY_D, KEY_W, KEY_S })
                               .set(Speed{ 200 })
                               .add<Player>()
                               .add<Gun>();

    // world.script_run_file("./games/rtype/scripts/player.flecs");

    world.system<const engine::Position>()
        .with<Player>()
        .with<Gun>()
        .kind(flecs::OnUpdate)
        .each([](flecs::iter &it, size_t, const engine::Position &pos) {
            if (IsKeyPressed(KEY_SPACE)) {
                it.world()
                    .entity()
                    .set<engine::Position>({ pos.x + 100, pos.y + 50 })
                    .set(engine::Velocity{ 1200 })
                    .set(engine::Rectangle{ 20, 20 })
                    .set(Color{ 0, 255, 0, 255 })
                    .set(engine::DespawnIn::from_seconds(4));
            }
        });

    world.app().target_fps(120).enable_rest().enable_stats().run();
}
