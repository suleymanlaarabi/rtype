#include "simulation.hpp"
#include "components.hpp"
#include "resources.hpp"
#include "spatial_grid.hpp"

namespace simulation {

void simulation::import() {
    ecs::component<Ship>();
    ecs::component<Projectile>(ecs::component_options<Projectile>{
        .hooks = {
            .on_remove =
                [](ecs_entity_t, Projectile &) {
                    --ecs::resource<SimulationStats>().active_projectiles;
                },
            .on_add = [](ecs_entity_t,
                         Projectile &) { ++ecs::resource<SimulationStats>().active_projectiles; },
        },
    });
    ecs::component<Team>();
    ecs::component<Health>();
    ecs::component<ShipConfig>(ecs::component_options<ShipConfig>{ .inheritance =
                                                                       EcsInheritShared });
    ecs::component<Target>();
    ecs::component<AIState>();
    ecs::component<Steering>();
    ecs::component<WeaponConfig>(ecs::component_options<WeaponConfig>{ .inheritance =
                                                                           EcsInheritShared });
    ecs::component<WeaponState>();
    ecs::component<SphereCollider>();
    ecs::component<ProjectileData>();
    ecs::component<PreviousPosition3d>();
    ecs::component<FreeCameraController>();

    ecs::set_resource(SimulationConfig{});
    ecs::set_resource(SimulationStats{});
    static SpatialGrid spatial_grid;
    static DamageQueue damage_queue;
    damage_queue.events.reserve(4096);
    ecs::set_resource(SpatialGridResource{ &spatial_grid });
    ecs::set_resource(DamageQueueResource{ &damage_queue });

    const ecs_system_id_t previous_position = register_projectile_previous_position();
    const ecs_system_id_t grid_system = register_spatial_grid(previous_position);
    const ecs_system_id_t targeting_system = register_targeting(grid_system);
    const ecs_system_id_t steering_system = register_steering(targeting_system);
    register_weapons(steering_system);
    const ecs_system_id_t collision_system = register_projectile_collision();
    const ecs_system_id_t damage_system = register_damage(collision_system);
    register_debug(damage_system);
}

} // namespace simulation
