#include "gameplay.hpp"
#include "core.hpp"
#include "rendering.hpp"
#include "siecs.h"
#include <siecs_spatial.h>

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
                 ecs::res<const engine::Keyboard> keyboard,
                 ecs::res<const DeltaTime> delta) {
            constexpr float move_speed = 8.0f;
            constexpr float rotation_speed = 1.5f;

            const float forward = static_cast<float>(keyboard->down(engine::Key::W)) -
                                  static_cast<float>(keyboard->down(engine::Key::S));
            const float strafe = static_cast<float>(keyboard->down(engine::Key::D)) -
                                 static_cast<float>(keyboard->down(engine::Key::A));
            const float vertical = static_cast<float>(keyboard->down(engine::Key::E)) -
                                   static_cast<float>(keyboard->down(engine::Key::Q));
            const float yaw = rotation.y;
            const float sin_yaw = std::sin(yaw);
            const float cos_yaw = std::cos(yaw);

            position.x += (forward * sin_yaw + strafe * cos_yaw) * move_speed * delta->value;
            position.y += vertical * move_speed * delta->value;
            position.z += (forward * cos_yaw - strafe * sin_yaw) * move_speed * delta->value;

            rotation.y += (static_cast<float>(keyboard->down(engine::Key::Right)) -
                           static_cast<float>(keyboard->down(engine::Key::Left))) *
                          rotation_speed * delta->value;
            rotation.x += (static_cast<float>(keyboard->down(engine::Key::Down)) -
                           static_cast<float>(keyboard->down(engine::Key::Up))) *
                          rotation_speed * delta->value;
        });

    ecs::system("Move")
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
                      .add<Velocity3d>()
                      .set(
                          engine::Cuboid::splat(0.15f),
                          engine::Color::yellow(),
                          engine::Bloom(2.0f),
                          engine::DespawnIn::seconds(6)
                      )
                      .abstract();

    ecs::system("SpawnProjectile")
        .phase(EcsOnUpdate)
        .each([bullet](
                  ecs::entity entity,
                  const Gun &gun,
                  const GlobalPosition3d &position,
                  const GlobalRotation3d &rotation,
                  ecs::res<const engine::Keyboard> keyboard
              ) {
            if (keyboard->down(gun.key)) {
                const float sin_y = std::sin(rotation.y);
                const float cos_y = std::cos(rotation.y);
                const float sin_z = std::sin(rotation.z);
                const float cos_z = std::cos(rotation.z);
                const float forward_x = cos_z * cos_y;
                const float forward_y = sin_z * cos_y;
                const float forward_z = -sin_y;

                ecs::entity::instantiate(bullet).set(
                    Velocity3d{
                        40.0f * forward_x,
                        40.0f * forward_y,
                        40.0f * forward_z,
                    },
                    Position3d{
                        position.x + 0.5f * forward_x,
                        position.y + 0.5f * forward_y,
                        position.z + 0.5f * forward_z,
                    }
                );
                entity.set(engine::DisableFor::seconds(0.15));
            }
        });
}

} // namespace rtype
