#include "gameplay.hpp"
#include "core.hpp"
#include "rendering.hpp"
#include "siecs.h"
#include <siecs_spatial.h>

namespace rtype {

void gameplay::import() {
    ecs::component<Player>();
    ecs::component<Gun>();
    ecs::component<MoveInput>();

    ecs::system("IntegrateVelocity")
        .each(
            [](Position3d &position, const Velocity3d &velocity, ecs::res<const DeltaTime> delta) {
                position.x += velocity.x * delta->value;
                position.y += velocity.y * delta->value;
            }
        );

    ecs::system("MovePlayer")
        .require<Player>()
        .phase(EcsPreUpdate)
        .each([](const MoveInput &input,
                 Velocity3d &velocity,
                 ecs::res<const engine::Keyboard> keyboard) {
            velocity.x = 0;
            velocity.y = 0;

            if (keyboard->down(input.left)) {
                velocity.x = -input.speed;
            }
            if (keyboard->down(input.right)) {
                velocity.x = input.speed;
            }
            if (keyboard->down(input.up)) {
                velocity.y = input.speed;
            }
            if (keyboard->down(input.down)) {
                velocity.y = -input.speed;
            }
        });

    auto bullet = ecs::entity::create()
                      .set(
                          Velocity3d(12.0f, 0.0f),
                          engine::Cuboid::splat(0.2f),
                          engine::Color::green(),
                          engine::DespawnIn::seconds(1)
                      )
                      .abstract();

    ecs::system("SpawnProjectile")
        .phase(EcsOnUpdate)
        .each([bullet](
                  ecs::entity entity,
                  const Gun &gun,
                  const GlobalPosition3d &position,
                  ecs::res<const engine::Keyboard> keyboard
              ) {
            if (keyboard->down(gun.key)) {
                ecs::entity::instantiate(bullet).set(
                    Position3d{ position.x + 0.5f, position.y, position.z }
                );
                entity.set(engine::DisableFor::seconds(0.15));
            }
        });
}

} // namespace rtype
