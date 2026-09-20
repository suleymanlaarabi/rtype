#pragma once

#include <siecs.h>

namespace simulation {

struct simulation {
    static void import();
};

void spawn_battle();

ecs_system_id_t register_projectile_previous_position();
ecs_system_id_t register_spatial_grid(ecs_system_id_t previous_position);
ecs_system_id_t register_targeting(ecs_system_id_t grid_system);
ecs_system_id_t register_steering(ecs_system_id_t targeting_system);
ecs_system_id_t register_weapons(ecs_system_id_t steering_system);
ecs_system_id_t register_projectile_collision();
ecs_system_id_t register_damage(ecs_system_id_t collision_system);
void register_debug(ecs_system_id_t damage_system);

} // namespace simulation
