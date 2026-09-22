#include "core.hpp"
#include "debug.hpp"
#include "gameplay.hpp"
#include <siecs.h>
#include <siecs_rest.h>
#include <siecs_spatial.h>
#include <sigpu.h>

#include <numbers>

int main() {
    ecs::init({ .target_fps = 120, .worker_threads = 4 });

    ecs::import<engine::core>();
    ecs::import<sigpu>();
    ecs::set_resource(Shadows{ .enabled = false });
#ifndef NDEBUG
    ecs::import<engine::debug>();
#endif
    ecs::import<rtype::gameplay>();
    // ecs::import<sirest>();

    ecs::entity::create("rtype::camera")
        .add<rtype::CameraController>()
        .set(
            Position3d(-4.6f, 4.8f, -8.2f),
            Rotation3d(-0.47f, std::numbers::pi_v<float> + 0.51f, 0.0f),
            Camera(40.0f)
        );

    ecs::entity gun = ecs::entity::create().add<Position3d>().set(rtype::Gun(Key::Space));

#ifndef NDEBUG
    gun.add<engine::DebugTransform>();
#endif

    ecs::entity wing =
        ecs::entity::create()
            .add<Position3d>()
            .set(Cuboid(0.4, 0.1, 1.2), Color{ 225, 225, 235, 255 })
            .children(
                ecs::entity::create()
                    .set(Position3d(0, 0, 0.5), Cuboid(0.42, 0.12, 0.2), Color::brown()),
                ecs::entity::create()
                    .set(Position3d(0, 0, -0.5), Cuboid(0.35, 0.25, 0.2), Color::gray()),
                ecs::entity::create()
                    .set(Position3d(0, 0, 0.55), Cuboid(0.2, 0.05, 0.2), Color::red(), Bloom(1)),
                gun
            )
            .abstract();

    ecs::entity spaceship = ecs::entity::create()
                                .add<Position3d, Rotation3d, Velocity3d, rtype::Player>()
                                .set(Cuboid(1, 0.3f, 2.2f), Color{ 225, 225, 235, 255 })
                                .children(
                                    ecs::entity::instantiate(wing).set(Position3d(0.7, 0, 0)),
                                    ecs::entity::instantiate(wing).set(Position3d(-0.7, 0, 0))
                                )
                                .abstract();

    ecs::entity player = ecs::entity::instantiate(spaceship).set(Position3d(0, 0, 0));

    ecs::run();
}
