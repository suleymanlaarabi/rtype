#include "core.hpp"
#include "debug.hpp"
#include "gameplay.hpp"
#include "rendering.hpp"
#include <siecs.h>
#include <siecs_rest.h>
#include <siecs_spatial.h>

#include <numbers>

int main() {
    ecs::init({ .target_fps = 120, .worker_threads = 4 });

    ecs::import<engine::core>();
    ecs::import<engine::rendering>();
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
            engine::Camera(40.0f)
        );

    // ecs::entity tree =
    //     ecs::entity::create()
    //         .set(Position3d(0, 0, 0), engine::Cuboid(0.5, 3, 0.5), engine::Color::brown())
    //         .children(
    //             ecs::entity::create()
    //                 .set(engine::Cuboid::splat(1.8), Position3d(0, 1, 0),
    //                 engine::Color::green()),
    //             ecs::entity::create()
    //                 .set(engine::Cuboid::splat(1.4), Position3d(0, 1.5, 0),
    //                 engine::Color::green()),
    //             ecs::entity::create()
    //                 .set(engine::Cuboid::splat(1), Position3d(0, 2, 0), engine::Color::green())
    //         )
    //         .abstract();

    // ecs::entity::instantiate(tree).set(Position3d(2, 0, 0));
    // ecs::entity::instantiate(tree).set(Position3d(4, 0, 0));

    ecs::entity gun = ecs::entity::create().add<Position3d>().set(rtype::Gun(engine::Key::Space));

#ifndef NDEBUG
    gun.add<engine::DebugTransform>();
#endif

    ecs::entity wing = ecs::entity::create()
                           .add<Position3d>()
                           .set(engine::Cuboid(0.4, 0.1, 1.2), engine::Color{ 225, 225, 235, 255 })
                           .children(
                               ecs::entity::create().set(
                                   Position3d(0, 0, 0.5),
                                   engine::Cuboid(0.42, 0.12, 0.2),
                                   engine::Color::brown()
                               ),
                               ecs::entity::create().set(
                                   Position3d(0, 0, -0.5),
                                   engine::Cuboid(0.35, 0.25, 0.2),
                                   engine::Color::gray()
                               ),
                               ecs::entity::create().set(
                                   Position3d(0, 0, 0.55),
                                   engine::Cuboid(0.2, 0.05, 0.2),
                                   engine::Color::red(),
                                   engine::Bloom(1)
                               ),
                               gun
                           )
                           .abstract();

    ecs::entity spaceship =
        ecs::entity::create()
            .add<Position3d, Rotation3d, Velocity3d, rtype::Player>()
            .set(engine::Cuboid(1, 0.3f, 2.2f), engine::Color{ 225, 225, 235, 255 })
            .children(
                ecs::entity::instantiate(wing).set(Position3d(0.7, 0, 0)),
                ecs::entity::instantiate(wing).set(Position3d(-0.7, 0, 0))
            )
            .abstract();
    ecs::entity player = ecs::entity::instantiate(spaceship).set(Position3d(0, 0, 0));

#ifndef NDEBUG
    player.add<engine::DebugTransform>();
#endif

    ecs::run();
}
