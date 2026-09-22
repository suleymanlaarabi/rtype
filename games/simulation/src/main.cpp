#include "simulation.hpp"

#include "core.hpp"
#include "debug.hpp"
#include "resources.hpp"
#include <sigpu.h>

#include <siecs.h>

int main() {
    ecs::init({ .target_fps = 120, .worker_threads = 8 });

    ecs::import<engine::core>();
    ecs::import<sigpu>({ .width = 1280, .height = 720 });
    ecs::set_resource(Shadows{ .enabled = false });
    ecs::set_resource(Multisampling{ .samples = 1 });
    ecs::set_resource(BloomSettings{ .enabled = false });
#ifndef NDEBUG
    ecs::import<engine::debug>();
#endif
    ecs::import<simulation::simulation>();
    ecs::set_resource(
        simulation::SimulationConfig{
            .ships_per_team = 4000,
            .seed = 1,
            .max_active_projectiles = 8000,
            .arena_radius = 850.0f,
            .grid_cell_size = 50.0f,
        }
    );
    simulation::spawn_battle();

    ecs::run();
}
