#include "components.hpp"
#include "resources.hpp"
#include "simulation.hpp"
#include "spatial_grid.hpp"

#include "rendering.hpp"

#include <algorithm>
#include <cmath>

namespace simulation {

namespace {

bool segment_hits_sphere(
    const PreviousPosition3d &start,
    const Position3d &end,
    const GridEntry &sphere,
    float projectile_radius
) {
    const float dx = end.x - start.x;
    const float dy = end.y - start.y;
    const float dz = end.z - start.z;
    const float length_squared = dx * dx + dy * dy + dz * dz;
    const float to_center_x = sphere.x - start.x;
    const float to_center_y = sphere.y - start.y;
    const float to_center_z = sphere.z - start.z;
    const float projection =
        length_squared == 0.0f
            ? 0.0f
            : std::clamp(
                  (to_center_x * dx + to_center_y * dy + to_center_z * dz) / length_squared,
                  0.0f,
                  1.0f
              );
    const float closest_x = start.x + dx * projection;
    const float closest_y = start.y + dy * projection;
    const float closest_z = start.z + dz * projection;
    const float hit_x = sphere.x - closest_x;
    const float hit_y = sphere.y - closest_y;
    const float hit_z = sphere.z - closest_z;
    const float radius = sphere.radius + projectile_radius;
    return hit_x * hit_x + hit_y * hit_y + hit_z * hit_z <= radius * radius;
}

} // namespace

ecs_system_id_t register_projectile_previous_position() {
    return ecs::system("Simulation.SaveProjectilePreviousPosition")
        .phase(EcsPreUpdate)
        .each([](const Projectile &, const Position3d &position, PreviousPosition3d &previous) {
            previous = { position.x, position.y, position.z };
        });
}

ecs_system_id_t register_projectile_collision() {
    return ecs::system("Simulation.ProjectileCollision")
        .phase(EcsPostUpdate)
        .immediate()
        .each([](ecs::entity entity,
                 const Projectile &,
                 const Position3d &position,
                 const PreviousPosition3d &previous,
                 const ProjectileData &projectile,
                 ecs::res<const SpatialGridResource> resource,
                 ecs::res<DamageQueueResource> damage_queue,
                 ecs::res<SimulationStats> stats) {
            const float dx = position.x - previous.x;
            const float dy = position.y - previous.y;
            const float dz = position.z - previous.z;
            const float segment_length = std::sqrt(dx * dx + dy * dy + dz * dz);
            const float search_radius = segment_length + projectile.radius + 1.2f;
            ecs_entity_t hit = 0;

            resource->value->for_nearby(position, search_radius, [&](const GridEntry &candidate) {
                if (hit != 0 || candidate.entity == projectile.owner ||
                    candidate.team == projectile.team) {
                    return;
                }
                if (segment_hits_sphere(previous, position, candidate, projectile.radius)) {
                    hit = candidate.entity;
                }
            });

            if (hit == 0) {
                return;
            }

            damage_queue->value->events.push_back({ projectile.owner, hit, projectile.damage });
            ++stats->hits;
            entity.kill();
        });
}

} // namespace simulation
