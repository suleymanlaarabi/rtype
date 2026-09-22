#include "gameplay.hpp"
#include "core.hpp"
#include "siecs.h"
#include <siecs_spatial.h>
#include <sigpu.h>

#include <cmath>

namespace rtype {

void gameplay::import() {
    ecs::component<Player>();
    ecs::component<CameraController>();
    ecs::component<Gun>();
    ecs::component<MoveInput>().with<Velocity3d>();

    ecs::system("CameraController")
        .phase(EcsPreUpdate)
        .each([](const CameraController &,
                 Position3d &position,
                 Rotation3d &rotation,
                 ecs::res<const Keyboard> keyboard,
                 ecs::res<const DeltaTime> delta) {
            constexpr float move_speed = 8.0f;
            constexpr float rotation_speed = 1.5f;

            const float forward = static_cast<float>(keyboard->down(Key::S)) -
                                  static_cast<float>(keyboard->down(Key::W));
            const float strafe = static_cast<float>(keyboard->down(Key::A)) -
                                 static_cast<float>(keyboard->down(Key::D));
            const float vertical = static_cast<float>(keyboard->down(Key::E)) -
                                   static_cast<float>(keyboard->down(Key::Q));
            const float yaw = rotation.yaw;
            const float sin_yaw = std::sin(yaw);
            const float cos_yaw = std::cos(yaw);

            position.x += (forward * sin_yaw + strafe * cos_yaw) * move_speed * delta->value;
            position.y += vertical * move_speed * delta->value;
            position.z += (forward * cos_yaw - strafe * sin_yaw) * move_speed * delta->value;

            rotation.yaw += (static_cast<float>(keyboard->down(Key::Right)) -
                             static_cast<float>(keyboard->down(Key::Left))) *
                            rotation_speed * delta->value;
            rotation.pitch -= (static_cast<float>(keyboard->down(Key::Down)) -
                               static_cast<float>(keyboard->down(Key::Up))) *
                              rotation_speed * delta->value;
        });

    ecs::system("Move")
        .phase(EcsPreUpdate)
        .each([](const MoveInput &input, Velocity3d &velocity, ecs::res<const Keyboard> keyboard) {
            velocity.x = 0;
            velocity.y = 0;

            if (keyboard->down(static_cast<Key>(input.left))) {
                velocity.x = -input.speed;
            }
            if (keyboard->down(static_cast<Key>(input.right))) {
                velocity.x = input.speed;
            }
            if (keyboard->down(static_cast<Key>(input.up))) {
                velocity.y = input.speed;
            }
            if (keyboard->down(static_cast<Key>(input.down))) {
                velocity.y = -input.speed;
            }
        });

    auto bullet =
        ecs::entity::create()
            .add<Velocity3d>()
            .set(Cuboid::splat(0.15f), Color::yellow(), Bloom(2.0f), engine::DespawnIn::seconds(6))
            .abstract();

    ecs::system("SpawnProjectile")
        .phase(EcsPostUpdate)
        .each([bullet](
                  ecs::entity entity,
                  const Gun &gun,
                  const GlobalPosition3d &position,
                  const GlobalOrientation3d &orientation,
                  ecs::res<const Keyboard> keyboard
              ) {
            if (keyboard->down(gun.key)) {
                const Direction3d forward = sispatial_forward_3d(&orientation);

                ecs::entity::instantiate(bullet).set(
                    Velocity3d{
                        40.0f * forward.x,
                        40.0f * forward.y,
                        40.0f * forward.z,
                    },
                    Position3d{
                        position.x + 0.5f * forward.x,
                        position.y + 0.5f * forward.y,
                        position.z + 0.5f * forward.z,
                    }
                );
                entity.set(engine::DisableFor::seconds(0.15));
            }
        });
}

} // namespace rtype
