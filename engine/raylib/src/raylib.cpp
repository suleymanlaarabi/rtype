#include "raylib.hpp"
#include "rendering.hpp"
#include "spatial.hpp"
#include <raylib.h>

namespace engine {

namespace {

::Color to_raylib_color(const engine::Color &color) {
    return ::Color{ color.r, color.g, color.b, color.a };
}

} // namespace

void raylib::import() {
    ecs::set_resource(
        WindowConfig{
            .width = 800,
            .height = 600,
            .title = "R-Type",
        }
    );

    const auto &config = ecs::resource<const WindowConfig>();
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_HIGHDPI);

    InitWindow(config.width, config.height, config.title);
    SetTargetFPS(240);

    ecs::system().phase(EcsPreUpdate).immediate().each([] {
        if (WindowShouldClose()) {
            ecs::quit();
        }
        BeginDrawing();
        ClearBackground(::Color{ 0, 0, 0, 255 });
    });

    const auto render_circle =
        ecs::system()
            .phase(EcsPostUpdate)
            .immediate()
            .each([](const Position &pos, const Circle &circle, const Color &color) {
                DrawCircle(pos.x, pos.y, circle.radius, to_raylib_color(color));
            });

    const auto render_rectangle =
        ecs::system()
            .phase(EcsPostUpdate)
            .after(render_circle)
            .immediate()
            .each([](const Position &pos, const Rectangle &rectangle, const Color &color) {
                DrawRectangle(
                    pos.x,
                    pos.y,
                    rectangle.width,
                    rectangle.height,
                    to_raylib_color(color)
                );
            });

    ecs::system().phase(EcsPostUpdate).after(render_rectangle).immediate().each([] {
        EndDrawing();
        if (WindowShouldClose()) {
            ecs::quit();
        }
    });
}

void raylib::fini() { CloseWindow(); }

} // namespace engine
