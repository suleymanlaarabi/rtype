#pragma once

#include <siecs.h>

#include <cstdint>

namespace simulation {

struct Ship {};
struct Projectile {};

struct Team {
    uint8_t id;
};

struct Health {
    float current;
    float maximum;
};

struct ShipConfig {
    float max_speed;
    float acceleration;
    float turn_speed;
    float preferred_distance;
    float separation_radius;
    float separation_weight;
    float seek_weight;
    float orbit_weight;
    float boundary_weight;
};

struct Target {
    ecs_entity_t entity = 0;
};

enum class Behavior : uint8_t {
    Search,
    Pursue,
    Attack,
    Evade,
};

struct AIState {
    Behavior behavior;
    float orbit_sign;
};

struct Steering {
    float desired_x;
    float desired_y;
    float desired_z;
};

struct WeaponConfig {
    float fire_interval;
    float projectile_speed;
    float projectile_damage;
    float projectile_lifetime;
    float range;
    float aim_cos_threshold;
    float muzzle_x;
    float muzzle_y;
    float muzzle_z;
};

struct WeaponState {
    float cooldown;
};

struct SphereCollider {
    float radius;
};

struct ProjectileData {
    ecs_entity_t owner;
    uint8_t team;
    float damage;
    float radius;
};

struct PreviousPosition3d {
    float x;
    float y;
    float z;
};

struct FreeCameraController {};

} // namespace simulation
