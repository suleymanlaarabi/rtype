#pragma once

#include <siecs.h>

#include <array>
#include <cstdint>
#include <vector>

namespace simulation {

class SpatialGrid;

struct SimulationConfig {
    uint32_t ships_per_team = 50;
    uint32_t seed = 1;
    uint32_t max_active_projectiles = 512;
    float arena_radius = 120.0f;
    float grid_cell_size = 12.0f;
};

struct SimulationStats {
    std::array<uint32_t, 2> team_alive{};
    uint32_t active_projectiles = 0;
    uint64_t shots_fired = 0;
    uint64_t hits = 0;
    uint64_t ships_destroyed = 0;
    uint8_t winner = 2;
};

struct DamageEvent {
    ecs_entity_t source;
    ecs_entity_t target;
    float damage;
};

struct DamageQueue {
    std::vector<DamageEvent> events;
};

struct SpatialGridResource {
    SpatialGrid *value;
};

struct DamageQueueResource {
    DamageQueue *value;
};

struct ProjectilePrefabs {
    ecs_entity_t blue;
    ecs_entity_t red;
};

} // namespace simulation
