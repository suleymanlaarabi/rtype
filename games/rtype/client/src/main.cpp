#include "core.hpp"
#include "gameplay.hpp"
#include "rendering.hpp"
#include <siecs.h>
#include <siecs_rest.h>
#include <siecs_spatial.h>

int main() {
    ecs::init({ .target_fps = 120, .worker_threads = 4 });

    ecs::import<engine::core>();
    ecs::import<engine::rendering>();
    ecs::import<rtype::gameplay>();
    ecs::import<sirest>();

    ecs::set_resource(engine::Sky{ .color = engine::Color::lblue() });

    ecs::entity::create()
        .add<Velocity3d>()
        .set(
            Position3d(0, 0, 0.0f),
            engine::Cuboid(1.0f, 1.0f, 0.5f),
            engine::Color(255, 0, 0, 255),
            rtype::MoveInput{
                .left = engine::Key::A,
                .right = engine::Key::D,
                .up = engine::Key::W,
                .down = engine::Key::S,
                .speed = 2.0f,
            }
        )
        .add<rtype::Player>()
        .children(
            ecs::entity::create().set(
                rtype::Gun(engine::Key::Space),
                Position3d(0.7f, 0.0f, 0.0f),
                engine::Color::blue(),
                engine::Cuboid(0.5f, 0.2f, 0.35f)
            )
        );
    ecs::run();
}
