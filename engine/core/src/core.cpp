#include "core.hpp"
#include "physics.hpp"
#include "spatial.hpp"
#include <cstddef>
#include <cstdio>
#include <raylib.h>

using namespace engine;

engine_module::engine_module(flecs::world &world) {
    world.module<module>("engine");
    world.import<spatial>();
    world.import<physics>();
    world.component<WindowConfig>();
    world.component<Circle>().member<float>("radius");
    world.component<engine::Rectangle>().member<float>("width").member<float>("height");
    world.component<engine::DespawnIn>().member<float>("elapsed").member<float>("duration");
    world.component<Color>()
        .member<unsigned char>("r")
        .member<unsigned char>("g")
        .member<unsigned char>("b")
        .member<unsigned char>("a")
        .child_of(world.entity("::engine"));

    world.set<WindowConfig>({});

    const auto &config = world.get<WindowConfig>();
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    InitWindow(config.width, config.height, config.title.c_str());
    SetTargetFPS(240);

    world.system("BeginFrame").kind(flecs::PreUpdate).run([](flecs::iter &it) {
        if (WindowShouldClose()) {
            it.world().quit();
        }
        BeginDrawing();
        ClearBackground(BLACK);
    });

    world.system().kind(flecs::OnUpdate).each([](flecs::iter &it, size_t i, DespawnIn &despawnIn) {
        if (despawnIn.timer.tick(it.delta_time())) {
            it.entity(i).destruct();
        }
    });

    world.system<const Position, const Circle, const Color>("RenderCircle")
        .kind(flecs::PostUpdate)
        .each([](const Position &pos, const Circle &circle, const Color &color) {
            DrawCircle(pos.x, pos.y, circle.radius, color);
        });

    world.system<const Position, const engine::Rectangle, const Color>("RenderRectangle")
        .kind(flecs::PostUpdate)
        .each([](const Position &pos, const engine::Rectangle &rectangle, const Color &color) {
            DrawRectangle(pos.x, pos.y, rectangle.width, rectangle.height, color);
        });

    world.system("EndFrame").kind(flecs::PostUpdate).run([](flecs::iter &it) {
        EndDrawing();
        if (WindowShouldClose()) {
            it.world().quit();
        }
    });

    world.atfini([](ecs_world_t *, void *) {
        if (IsWindowReady()) {
            CloseWindow();
        }
    });
}

namespace engine {

bool Timer::tick(float delta) {
    this->elapsed += delta;
    if (this->duration <= this->elapsed) {
        this->elapsed = 0;
        return true;
    }
    return false;
}

Timer Timer::from_seconds(float seconds) { return Timer{ .elapsed = 0, .duration = seconds }; }

DespawnIn DespawnIn::from_seconds(float seconds) {
    return DespawnIn{ .timer = { .elapsed = 0, .duration = seconds } };
}

} // namespace engine
