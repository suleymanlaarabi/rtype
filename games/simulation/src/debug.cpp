#include "components.hpp"
#include "resources.hpp"
#include "simulation.hpp"

#include <sigpu.h>

#include <siecs_spatial.h>

#include <cmath>
#include <iostream>

namespace simulation {

void register_debug(ecs_system_id_t damage_system) {
    ecs::system("Simulation.FreeCamera")
        .phase(EcsPreUpdate)
        .each([](const FreeCameraController &,
                 Position3d &position,
                 Rotation3d &rotation,
                 ecs::res<const Keyboard> keyboard,
                 ecs::res<const DeltaTime> delta) {
            constexpr float move_speed = 35.0f;
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

    ecs::system("Simulation.BattleResult")
        .phase(EcsPostUpdate)
        .after(damage_system)
        .immediate()
        .each([](ecs::res<SimulationStats> stats) {
            if (stats->winner != 2 || (stats->team_alive[0] != 0 && stats->team_alive[1] != 0)) {
                return;
            }

            stats->winner = stats->team_alive[0] == 0 ? 1 : 0;
            std::cout << "Team " << static_cast<int>(stats->winner) << " wins after "
                      << stats->ships_destroyed << " ships destroyed.\n";
        });
}

} // namespace simulation
