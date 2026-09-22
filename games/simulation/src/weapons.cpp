#include "components.hpp"
#include "math.hpp"
#include "resources.hpp"
#include "simulation.hpp"
#include "spatial_grid.hpp"

#include "core.hpp"
#include <algorithm>

namespace simulation {

ecs_system_id_t register_weapons(ecs_system_id_t steering_system) {
    const ecs_system_id_t cooldown =
        ecs::system("Simulation.WeaponCooldown")
            .phase(EcsPreUpdate)
            .after(steering_system)
            .each([](WeaponState &weapon, ecs::res<const DeltaTime> delta) {
                weapon.cooldown = std::max(0.0f, weapon.cooldown - delta->value);
            });

    return ecs::system("Simulation.WeaponFire")
        .phase(EcsPreUpdate)
        .after(cooldown)
        .each([](ecs::entity entity,
                 const Team &team,
                 const Target &target,
                 const Position3d &position,
                 const Rotation3d &rotation,
                 const Velocity3d &velocity,
                 const WeaponConfig &config,
                 WeaponState &weapon,
                 ecs::res<const SpatialGridResource> resource,
                 ecs::res<const SimulationConfig> simulation_config,
                 ecs::res<const ProjectilePrefabs> projectile_prefabs,
                 ecs::res<SimulationStats> stats) {
            if (weapon.cooldown > 0.0f ||
                stats->active_projectiles >= simulation_config->max_active_projectiles) {
                return;
            }

            const GridEntry *target_entry = resource->value->find(target.entity);
            if (target_entry == nullptr) {
                return;
            }

            const float dx = target_entry->x - position.x;
            const float dy = target_entry->y - position.y;
            const float dz = target_entry->z - position.z;
            const float distance_squared = math::length_squared(dx, dy, dz);
            if (distance_squared > config.range * config.range || distance_squared == 0.0f) {
                return;
            }

            const Direction3d to_target = math::normalize(dx, dy, dz);
            const Direction3d forward = math::forward(rotation);
            const float alignment =
                forward.x * to_target.x + forward.y * to_target.y + forward.z * to_target.z;
            if (alignment < config.aim_cos_threshold) {
                return;
            }

            const ecs::entity prefab = ecs::entity::from(
                team.id == 0 ? projectile_prefabs->blue : projectile_prefabs->red
            );
            const Position3d muzzle{
                position.x + forward.x * config.muzzle_z + config.muzzle_x,
                position.y + forward.y * config.muzzle_z + config.muzzle_y,
                position.z + forward.z * config.muzzle_z,
            };
            ecs::entity::instantiate(prefab).add<Projectile>().set(
                muzzle,
                PreviousPosition3d{ muzzle.x, muzzle.y, muzzle.z },
                Velocity3d{
                    velocity.x + forward.x * config.projectile_speed,
                    velocity.y + forward.y * config.projectile_speed,
                    velocity.z + forward.z * config.projectile_speed,
                },
                ProjectileData{ entity.id(), team.id, config.projectile_damage, 0.18f },
                SphereCollider{ 0.18f },
                engine::DespawnIn::seconds(config.projectile_lifetime)
            );
            weapon.cooldown = config.fire_interval;
            ++stats->shots_fired;
        });
}

} // namespace simulation
