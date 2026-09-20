#include "components.hpp"
#include "prefabs.hpp"
#include "resources.hpp"
#include "simulation.hpp"

#include "rendering.hpp"

#include <siecs_spatial.h>

#include <cmath>
#include <numbers>

namespace simulation {

namespace {

uint32_t random_step(uint32_t value) {
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return value;
}

float random_range(uint32_t &state, float minimum, float maximum) {
    state = random_step(state);
    constexpr float scale = 1.0f / static_cast<float>(UINT32_MAX);
    return minimum + (maximum - minimum) * static_cast<float>(state) * scale;
}

void spawn_team(ecs::entity prefab, uint8_t team, const SimulationConfig &config) {
    uint32_t random = config.seed + team * 0x9e3779b9U;
    const float center_x = (team == 0 ? -1.0f : 1.0f) * config.arena_radius / 3.0f;
    const float spawn_x = config.arena_radius / 12.0f;
    const float spawn_y = config.arena_radius * 2.0f / 15.0f;
    const float spawn_z = config.arena_radius / 5.0f;
    const float yaw =
        team == 0 ? std::numbers::pi_v<float> / 2.0f : -std::numbers::pi_v<float> / 2.0f;

    for (uint32_t index = 0; index < config.ships_per_team; ++index) {
        const Position3d position{
            center_x + random_range(random, -spawn_x, spawn_x),
            random_range(random, -spawn_y, spawn_y),
            random_range(random, -spawn_z, spawn_z),
        };
        const float orbit_sign = random_range(random, 0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f;

        ecs::entity::instantiate(prefab).add<Ship>().set(
            Team{ team },
            Health{ 100.0f, 100.0f },
            Target{},
            AIState{ Behavior::Search, orbit_sign },
            Steering{},
            WeaponState{},
            SphereCollider{ 1.2f },
            position,
            Rotation3d(0.0f, yaw, 0.0f),
            Velocity3d{}
        );
    }
}

} // namespace

void spawn_battle() {
    const SimulationConfig &config = ecs::resource<const SimulationConfig>();
    const FighterPrefabs prefabs = create_fighter_prefabs();
    create_projectile_prefabs();
    spawn_team(prefabs.blue, 0, config);
    spawn_team(prefabs.red, 1, config);

    SimulationStats &stats = ecs::resource<SimulationStats>();
    stats.team_alive[0] = config.ships_per_team;
    stats.team_alive[1] = config.ships_per_team;

    const float camera_height = config.arena_radius * 4.0f / 25.0f;
    const float camera_distance = config.arena_radius * 3.0f / 4.0f;
    ecs::entity::create("simulation::camera")
        .add<FreeCameraController>()
        .set(
            Position3d(0.0f, camera_height, -camera_distance),
            Rotation3d(
                -std::atan2(camera_height, camera_distance),
                std::numbers::pi_v<float>,
                0.0f
            ),
            engine::Camera(52.0f)
        );
}

} // namespace simulation
