#include "components.hpp"
#include "resources.hpp"
#include "simulation.hpp"
#include "spatial_grid.hpp"

#include "rendering.hpp"

namespace simulation {

ecs_system_id_t register_spatial_grid(ecs_system_id_t previous_position) {
    const ecs_system_id_t clear = ecs::system("Simulation.ClearShipGrid")
                                      .phase(EcsPreUpdate)
                                      .after(previous_position)
                                      .immediate()
                                      .each([](ecs::res<SpatialGridResource> resource,
                                               ecs::res<const SimulationConfig> config) {
                                          resource->value->set_cell_size(config->grid_cell_size);
                                          resource->value->clear();
                                      });

    return ecs::system("Simulation.BuildShipGrid")
        .phase(EcsPreUpdate)
        .after(clear)
        .immediate()
        .each([](ecs::entity entity,
                 ecs::res<SpatialGridResource> resource,
                 const Ship &,
                 const Team &team,
                 const Position3d &position,
                 const SphereCollider &collider) {
            resource->value->insert(entity.id(), position, team.id, collider.radius);
        });
}

ecs_system_id_t register_targeting(ecs_system_id_t grid_system) {
    const ecs_system_id_t validate =
        ecs::system("Simulation.ValidateTargets")
            .phase(EcsPreUpdate)
            .after(grid_system)
            .each(
                [](Target &target, const Team &team, ecs::res<const SpatialGridResource> resource) {
                    if (target.entity == 0) {
                        return;
                    }

                    const GridEntry *entry = resource->value->find(target.entity);
                    if (entry == nullptr || entry->team == team.id) {
                        target.entity = 0;
                    }
                }
            );

    const ecs_system_id_t acquire =
        ecs::system("Simulation.AcquireTargets")
            .phase(EcsPreUpdate)
            .after(validate)
            .interval(0.3)
            .each([](ecs::entity entity,
                     Target &target,
                     const Team &team,
                     const Position3d &position,
                     ecs::res<const SpatialGridResource> resource,
                     ecs::res<const SimulationConfig> config) {
                if (target.entity != 0) {
                    return;
                }

                const float targeting_range = config->arena_radius * 4.0f / 5.0f;
                float best_distance_squared = targeting_range * targeting_range;
                ecs_entity_t best_target = 0;
                resource->value->for_populated_nearby(
                    position,
                    targeting_range,
                    [&](const GridEntry &candidate) {
                        if (candidate.entity == entity.id() || candidate.team == team.id) {
                            return;
                        }

                        const float dx = candidate.x - position.x;
                        const float dy = candidate.y - position.y;
                        const float dz = candidate.z - position.z;
                        const float distance_squared = dx * dx + dy * dy + dz * dz;
                        if (distance_squared < best_distance_squared) {
                            best_distance_squared = distance_squared;
                            best_target = candidate.entity;
                        }
                    }
                );
                target.entity = best_target;
            });

    return ecs::system("Simulation.CombatDecision")
        .phase(EcsPreUpdate)
        .after(acquire)
        .interval(0.2)
        .each([](AIState &state,
                 const Target &target,
                 const Position3d &position,
                 const ShipConfig &config,
                 ecs::res<const SpatialGridResource> resource) {
            const GridEntry *entry = resource->value->find(target.entity);
            if (entry == nullptr) {
                state.behavior = Behavior::Search;
                return;
            }

            const float dx = entry->x - position.x;
            const float dy = entry->y - position.y;
            const float dz = entry->z - position.z;
            const float distance_squared = dx * dx + dy * dy + dz * dz;
            const float pursue_distance = config.preferred_distance + 3.0f;
            state.behavior = distance_squared > pursue_distance * pursue_distance
                                 ? Behavior::Pursue
                                 : Behavior::Attack;
        });
}

} // namespace simulation
