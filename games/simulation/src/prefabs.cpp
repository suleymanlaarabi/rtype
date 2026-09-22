#include "prefabs.hpp"
#include "components.hpp"
#include "resources.hpp"

#include <sigpu.h>

#include <siecs_spatial.h>

namespace simulation {

namespace {

ecs::entity create_fighter_prefab(Color color) {
    const ShipConfig ship_config{
        .max_speed = 14.0f,
        .acceleration = 20.0f,
        .turn_speed = 2.4f,
        .preferred_distance = 16.0f,
        .separation_radius = 4.0f,
        .separation_weight = 1.6f,
        .seek_weight = 1.0f,
        .orbit_weight = 0.85f,
        .boundary_weight = 2.0f,
    };
    const WeaponConfig weapon_config{
        .fire_interval = 0.35f,
        .projectile_speed = 55.0f,
        .projectile_damage = 20.0f,
        .projectile_lifetime = 2.5f,
        .range = 45.0f,
        .aim_cos_threshold = 0.94f,
        .muzzle_x = 0.0f,
        .muzzle_y = 0.0f,
        .muzzle_z = 1.25f,
    };

    const ecs::entity wing =
        ecs::entity::create()
            .set(Cuboid(0.65f, 0.08f, 1.3f), color)
            .children(
                ecs::entity::create()
                    .set(Position3d(0.0f, 0.0f, 0.55f), Cuboid(0.22f, 0.08f, 0.3f), Color::yellow())
            )
            .abstract();

    return ecs::entity::create()
        .set(ship_config, weapon_config, Cuboid(0.9f, 0.3f, 2.3f), color)
        .children(
            ecs::entity::instantiate(wing).set(Position3d(0.72f, 0.0f, 0.0f)),
            ecs::entity::instantiate(wing).set(Position3d(-0.72f, 0.0f, 0.0f))
        )
        .abstract();
}

ecs::entity create_projectile_prefab(Color color) {
    return ecs::entity::create().set(Cuboid::splat(0.14f), color).abstract();
}

} // namespace

FighterPrefabs create_fighter_prefabs() {
    return {
        .blue = create_fighter_prefab(Color::lblue()),
        .red = create_fighter_prefab(Color::red()),
    };
}

void create_projectile_prefabs() {
    const ProjectilePrefabs prefabs{
        .blue = create_projectile_prefab(Color::lblue()).id(),
        .red = create_projectile_prefab(Color::red()).id(),
    };
    ecs::set_resource(prefabs);
}

} // namespace simulation
