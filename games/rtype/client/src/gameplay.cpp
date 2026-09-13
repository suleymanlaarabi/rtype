#include "gameplay.hpp"
#include "core.hpp"
#include "raylib.h"
#include "rendering.hpp"
#include "spatial.hpp"

namespace rtype {

void gameplay::import() {
    ecs::component<Player>();
    ecs::component<Gun>();
    ecs::component<Speed>();
    ecs::component<MoveInput>();

    ecs::system("MovePlayer")
        .require<Player>()
        .phase(EcsPreUpdate)
        .each([](MoveInput &input, engine::Velocity &vel) {
            vel = { 0 };

            if (IsKeyDown(input.left)) {
                vel.x = -input.speed;
            }
            if (IsKeyDown(input.right)) {
                vel.x = input.speed;
            }
            if (IsKeyDown(input.up)) {
                vel.y = -input.speed;
            }
            if (IsKeyDown(input.down)) {
                vel.y = input.speed;
            }
        });

    ecs::system("SpawnProjectile")
        .require<Gun>()
        .phase(EcsOnUpdate)
        .each([](const engine::Position &position) {
            if (IsKeyDown(KEY_SPACE)) {
                ecs::entity::create()
                    .set(engine::Position{ position.x + 100, position.y + 50 })
                    .set(engine::Velocity{ 1200, 0 })
                    .set(engine::Rectangle{ 20, 20 })
                    .set(engine::Color{ 0, 255, 0, 255 })
                    .set(engine::DespawnIn::from_seconds(4));
            }
        });
}

} // namespace rtype
