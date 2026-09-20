#include "simulation.hpp"

#include "core.hpp"
#include "debug.hpp"
#include "rendering.hpp"
#include "resources.hpp"

#include <siecs.h>

int main() {
    ecs::init({ .target_fps = 120, .worker_threads = 8 });

    ecs::import<engine::core>();
    ecs::import<engine::rendering>();
    ecs::set_resource(engine::Shadows{ .enabled = false });
    ecs::set_resource(engine::Multisampling{ .samples = 1 });
    ecs::set_resource(engine::BloomSettings{ .enabled = false });
#ifndef NDEBUG
    ecs::import<engine::debug>();
#endif
    ecs::import<simulation::simulation>();
    ecs::set_resource(
        simulation::SimulationConfig{
            .ships_per_team = 2000,
            .seed = 1,
            .max_active_projectiles = 3000,
            .arena_radius = 850.0f,
            .grid_cell_size = 50.0f,
        }
    );
    simulation::spawn_battle();

    ecs::run();
}
