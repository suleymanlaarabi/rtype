#include "components.hpp"
#include "math.hpp"
#include "resources.hpp"
#include "simulation.hpp"
#include "spatial_grid.hpp"

#include <cmath>

namespace simulation {

namespace {

void add_normalized(float &x, float &y, float &z, float dx, float dy, float dz, float weight) {
    const float distance_squared = math::length_squared(dx, dy, dz);
    if (distance_squared == 0.0f) {
        return;
    }

    const float inverse_distance = 1.0f / std::sqrt(distance_squared);
    x += dx * inverse_distance * weight;
    y += dy * inverse_distance * weight;
    z += dz * inverse_distance * weight;
}

} // namespace

ecs_system_id_t register_steering(ecs_system_id_t targeting_system) {
    const ecs_system_id_t intent =
        ecs::system("Simulation.SteeringIntent")
            .phase(EcsPreUpdate)
            .after(targeting_system)
            .each([](ecs::entity entity,
                     Steering &steering,
                     const Target &target,
                     const AIState &state,
                     const ShipConfig &config,
                     const Position3d &position,
                     ecs::res<const SimulationConfig> simulation_config,
                     ecs::res<const SpatialGridResource> resource) {
                float desired_x = 0.0f;
                float desired_y = 0.0f;
                float desired_z = 0.0f;
                const SpatialGrid &grid = *resource->value;
                const GridEntry *target_entry = grid.find(target.entity);

                if (target_entry == nullptr) {
                    add_normalized(
                        desired_x,
                        desired_y,
                        desired_z,
                        -position.x,
                        -position.y,
                        -position.z,
                        0.25f
                    );
                } else {
                    const float dx = target_entry->x - position.x;
                    const float dy = target_entry->y - position.y;
                    const float dz = target_entry->z - position.z;
                    const float distance_squared = math::length_squared(dx, dy, dz);
                    const float preferred_squared =
                        config.preferred_distance * config.preferred_distance;

                    const float seek_sign =
                        distance_squared < preferred_squared * 0.7f ? -1.0f : 1.0f;
                    add_normalized(
                        desired_x,
                        desired_y,
                        desired_z,
                        dx,
                        dy,
                        dz,
                        config.seek_weight * seek_sign
                    );

                    if (state.behavior == Behavior::Attack) {
                        add_normalized(
                            desired_x,
                            desired_y,
                            desired_z,
                            -dz * state.orbit_sign,
                            0.0f,
                            dx * state.orbit_sign,
                            config.orbit_weight
                        );
                    }
                }

                grid.for_nearby(position, config.separation_radius, [&](const GridEntry &neighbor) {
                    if (neighbor.entity == entity.id()) {
                        return;
                    }

                    const float dx = position.x - neighbor.x;
                    const float dy = position.y - neighbor.y;
                    const float dz = position.z - neighbor.z;
                    const float distance_squared = math::length_squared(dx, dy, dz);
                    const float separation_squared =
                        config.separation_radius * config.separation_radius;
                    if (distance_squared == 0.0f || distance_squared >= separation_squared) {
                        return;
                    }

                    const float weight =
                        config.separation_weight * (1.0f - distance_squared / separation_squared);
                    add_normalized(desired_x, desired_y, desired_z, dx, dy, dz, weight);
                });

                const float center_distance = math::length(position.x, position.y, position.z);
                if (center_distance > simulation_config->arena_radius) {
                    add_normalized(
                        desired_x,
                        desired_y,
                        desired_z,
                        -position.x,
                        -position.y,
                        -position.z,
                        config.boundary_weight
                    );
                }

                const float desired_length_squared =
                    math::length_squared(desired_x, desired_y, desired_z);
                if (desired_length_squared == 0.0f) {
                    return;
                }

                const Direction3d direction = math::normalize(desired_x, desired_y, desired_z);
                steering.desired_x = direction.x;
                steering.desired_y = direction.y;
                steering.desired_z = direction.z;
            });

    return ecs::system("Simulation.SteeringApply")
        .phase(EcsPreUpdate)
        .after(intent)
        .each([](const Steering &steering,
                 const ShipConfig &config,
                 Rotation3d &rotation,
                 Velocity3d &velocity,
                 ecs::res<const DeltaTime> delta) {
            const float target_yaw = std::atan2(steering.desired_x, steering.desired_z);
            const float target_pitch = -std::asin(steering.desired_y);
            const float rotation_delta = config.turn_speed * delta->value;
            rotation.yaw = math::approach_angle(rotation.yaw, target_yaw, rotation_delta);
            rotation.pitch = math::approach_angle(rotation.pitch, target_pitch, rotation_delta);

            const Direction3d forward = math::forward(rotation);
            const float velocity_delta = config.acceleration * delta->value;
            velocity.x = math::approach(velocity.x, forward.x * config.max_speed, velocity_delta);
            velocity.y = math::approach(velocity.y, forward.y * config.max_speed, velocity_delta);
            velocity.z = math::approach(velocity.z, forward.z * config.max_speed, velocity_delta);
        });
}

} // namespace simulation
