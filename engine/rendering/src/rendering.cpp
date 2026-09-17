#include "rendering.hpp"

#include "sigpu.h"

#include <SDL3/SDL.h>
#include <siecs_spatial.h>

#include <array>
#include <numbers>

namespace engine {

namespace {

sigpu_color_t to_sigpu(Color color) { return { color.r, color.g, color.b, color.a }; }

void set_camera(const Camera &camera) {
    sigpu_camera(
        camera.x,
        camera.y,
        camera.z,
        camera.target_x,
        camera.target_y,
        camera.target_z,
        camera.fov
    );
}

void set_sky(const Sky &sky) { sigpu_sky(to_sigpu(sky.color)); }

void set_sun(const Sun &sun) { sigpu_sun(sun.x, sun.y, sun.z, to_sigpu(sun.color), sun.intensity); }

void set_ambient(const AmbientLight &ambient) {
    sigpu_ambient(to_sigpu(ambient.color), ambient.intensity);
}

void set_fog(const Fog &fog) { sigpu_fog(to_sigpu(fog.color), fog.start, fog.end); }

void set_shadows(const Shadows &shadows) {
    sigpu_shadows(shadows.enabled);
    sigpu_shadow_distance(shadows.distance);
}

void set_multisampling(const Multisampling &multisampling) { sigpu_msaa(multisampling.samples); }

} // namespace

Color Color::green() { return Color{ 0, 255, 0, 255 }; }
Color Color::red() { return Color{ 255, 0, 0, 255 }; }
Color Color::blue() { return Color{ 0, 0, 255, 255 }; }
Color Color::lblue() { return Color{ 100, 100, 255, 255 }; }

Cuboid Cuboid::splat(float value) { return Cuboid{ value, value, value }; }

bool Keyboard::down(Key key) const { return keys[static_cast<std::size_t>(key)]; }

void rendering::import() {
    ecs::import<sispatial>();
    ecs::component<Color>();
    ecs::component<Cuboid>();

    ecs::set_resource(
        WindowConfig{
            .width = 800,
            .height = 600,
            .title = "R-Type",
        }
    );
    const auto &window = ecs::resource<const WindowConfig>();
    sigpu_init(window.title, window.width, window.height);

    ecs::resource_handle<Camera>({ .on_set = set_camera })
        .set(
            Camera{
                .x = 6.75f,
                .y = 6.75f,
                .z = -25.0f,
                .target_x = 6.75f,
                .target_y = 6.75f,
                .target_z = 0.0f,
                .fov = 40.0f,
            }
        );
    ecs::resource_handle<Sky>({ .on_set = set_sky }).set(Sky{ Color{ 13, 13, 20, 255 } });
    ecs::resource_handle<Sun>({ .on_set = set_sun })
        .set(
            Sun{
                .x = -1.0f,
                .y = -2.0f,
                .z = 1.0f,
                .color = Color{ 255, 245, 220, 255 },
                .intensity = 1.0f,
            }
        );
    ecs::resource_handle<AmbientLight>({ .on_set = set_ambient })
        .set(AmbientLight{ Color{ 255, 255, 255, 255 }, 0.2f });
    ecs::resource_handle<Fog>({ .on_set = set_fog })
        .set(Fog{ Color{ 13, 13, 20, 255 }, 0.0f, 0.0f });
    ecs::resource_handle<Shadows>({ .on_set = set_shadows }).set(Shadows{ false, 35.0f });
    ecs::resource_handle<Multisampling>({ .on_set = set_multisampling }).set(Multisampling{ 4 });
    ecs::set_resource(Keyboard{});

    ecs::system("BeginRendering")
        .phase(EcsPreUpdate)
        .immediate()
        .each([](ecs::res<Keyboard> keyboard) {
            if (!sigpu_begin_frame()) {
                ecs::quit();
            }

            constexpr std::array scancodes{
                SDL_SCANCODE_A,  SDL_SCANCODE_D,    SDL_SCANCODE_W,
                SDL_SCANCODE_S,  SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT,
                SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_SPACE,
            };
            const bool *state = SDL_GetKeyboardState(nullptr);
            for (std::size_t index = 0; index < scancodes.size(); ++index) {
                keyboard->keys[index] = state[scancodes[index]];
            }
        });

    ecs::system("RenderCuboids")
        .phase(EcsOnRender)
        .immediate()
        .each([](const GlobalPosition3d &position,
                 const GlobalRotation3d &rotation,
                 const GlobalScale3d &scale,
                 const Cuboid &cuboid,
                 const Color &color) {
            const float width = cuboid.width * scale.x;
            const float height = cuboid.height * scale.y;
            const float depth = cuboid.depth * scale.z;
            if (rotation.x == 0.0f && rotation.y == 0.0f && rotation.z == 0.0f) {
                sigpu_cube(
                    position.x,
                    position.y,
                    position.z,
                    width,
                    height,
                    depth,
                    to_sigpu(color)
                );
                return;
            }

            constexpr float degrees = 180.0f / std::numbers::pi_v<float>;
            sigpu_cube_rotated(
                position.x,
                position.y,
                position.z,
                width,
                height,
                depth,
                rotation.x * degrees,
                rotation.y * degrees,
                rotation.z * degrees,
                to_sigpu(color)
            );
        });

    ecs::system("EndRendering").phase(EcsPostRender).immediate().each([] { sigpu_end_frame(); });
    ecs::at_fini([] { sigpu_fini(); });
}

} // namespace engine
