#include "core.hpp"
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
    ecs::import<rtype::gameplay>();
    ecs::import<sirest>();

    ecs::set_resource(engine::Sky{ .color = engine::Color{ 45, 45, 42, 255 } });

    ecs::entity::create("rtype::camera")
        // .add<rtype::CameraController>()
        .set(Position3d(-4.6f, 4.8f, -8.2f), Rotation3d(0.47f, 0.51f, 0.0f), engine::Camera(40.0f));

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

    ecs::entity wing = ecs::entity::create()
                           .set(
                               Position3d(0, 0, 0),
                               engine::Cuboid(0.4, 0.1, 1.2),
                               engine::Color{ 225, 225, 235, 255 }
                           )
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
                                   engine::Bloom{ 1 }
                               )
                           )
                           .abstract();

    ecs::entity spaceship =
        ecs::entity::create()
            .add<Position3d, Rotation3d>()
            .set(engine::Cuboid(1, 0.3f, 2.2f), engine::Color{ 225, 225, 235, 255 })
            .children(
                ecs::entity::instantiate(wing).set(Position3d(0.7, 0, 0)),
                ecs::entity::instantiate(wing).set(Position3d(-0.7, 0, 0))
            );

    constexpr float ship_angle = 110.0f * (std::numbers::pi_v<float> / 180.0f);

    ecs::entity::instantiate(spaceship)
        .add<Velocity3d>()
        .set(
            rtype::MoveInput{
                .left = engine::Key::A,
                .right = engine::Key::D,
                .up = engine::Key::W,
                .down = engine::Key::S,
                .speed = 2.0f,
            },
            Rotation3d(0, ship_angle, 0)
        )
        .add<rtype::Player>()
        .children(
            ecs::entity::create().set(
                rtype::Gun(engine::Key::Space),
                Position3d(0.7f, 0.0f, 0.0f),
                Rotation3d(0.0f, std::numbers::pi_v<float> * 0.5f, 0.0f)
            ),
            ecs::entity::create().set(
                rtype::Gun(engine::Key::Space),
                Position3d(-0.7f, 0.0f, 0.0f),
                Rotation3d(0.0f, std::numbers::pi_v<float> * 0.5f, 0.0f)
            )
        );
    ecs::run();
}
