#include "rendering.hpp"

#include "sigpu_internal.h"

#include <SDL3/SDL.h>
#include <siecs_spatial.h>
#include <sireflect.h>

#include <array>
#include <cmath>

namespace engine {

namespace {

constexpr int default_multisampling = 4;
static_assert(sizeof(sigpu_shared_axis_instance_t) == 24);
static_assert(sizeof(sigpu_shared_rotated_instance_t) == 32);

constexpr sireflect_enum_desc_t engine_key_reflection = {
    .name = "EngineKey",
    .values = "{ "
              "A = 0, "
              "D = 1, "
              "W = 2, "
              "S = 3, "
              "Q = 4, "
              "Z = 5, "
              "E = 6, "
              "Left = 7, "
              "Right = 8, "
              "Up = 9, "
              "Down = 10, "
              "Space = 11 "
              "}",
    .size = sizeof(Key),
    .align = alignof(Key),
};

sigpu_color_t to_sigpu(Color color) { return { color.r, color.g, color.b, color.a }; }

template <typename T> struct table_field {
    T *data;
    std::ptrdiff_t stride;

    const T &operator[](uint32_t index) const { return data[index * stride]; }
};

template <typename T> table_field<T> field(ecs_iter_t *it, uint16_t index) {
    return {
        .data = static_cast<T *>(ecs_field(it, index)),
        .stride = ecs_field_is_shared(it, index) ? 0 : 1,
    };
}

struct packed_rotation {
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t w;
};

packed_rotation pack_rotation(const GlobalRotation3d &rotation) {
    const float half_x = rotation.x * 0.5f;
    const float half_y = rotation.y * 0.5f;
    const float half_z = rotation.z * 0.5f;
    const float sx = std::sin(half_x);
    const float cx = std::cos(half_x);
    const float sy = std::sin(half_y);
    const float cy = std::cos(half_y);
    const float sz = std::sin(half_z);
    const float cz = std::cos(half_z);
    return {
        static_cast<int16_t>(std::round((sx * cy * cz - cx * sy * sz) * 32767.0f)),
        static_cast<int16_t>(std::round((cx * sy * cz + sx * cy * sz) * 32767.0f)),
        static_cast<int16_t>(std::round((cx * cy * sz - sx * sy * cz) * 32767.0f)),
        static_cast<int16_t>(std::round((cx * cy * cz + sx * sy * sz) * 32767.0f)),
    };
}

bool visible(
    const GlobalPosition3d &position,
    float width,
    float height,
    float depth,
    float aspect
) {
    const float radius = 0.5f * std::sqrt(width * width + height * height + depth * depth);
    const sigpu_vec3_t center = { position.x, position.y, position.z };
    return sigpu_camera_visible(center, radius, aspect) ||
           (g_sigpu.shadows_enabled && sigpu_shadow_visible(center, radius));
}

void record_batch(
    sigpu_shared_batch_t *&batches,
    Uint32 &count,
    Uint32 &capacity,
    Uint32 first,
    Uint32 end,
    const sigpu_shared_material_t &material
) {
    if (first == end) {
        return;
    }
    if (count == capacity) {
        capacity = capacity ? capacity * 2 : 16;
        batches = static_cast<sigpu_shared_batch_t *>(
            SDL_realloc(batches, capacity * sizeof(sigpu_shared_batch_t))
        );
    }
    batches[count++] = { material, first, end - first };
}

void render_shared_cuboids(
    ecs_iter_t *it,
    table_field<const GlobalPosition3d> positions,
    table_field<const GlobalRotation3d> rotations,
    table_field<const GlobalScale3d> scales,
    table_field<const Cuboid> cuboids,
    table_field<const Color> colors,
    table_field<const Bloom> blooms,
    float aspect
) {
    const auto &cuboid = cuboids[0];
    const auto &color = colors[0];
    const float bloom = blooms.data ? std::fmax(blooms[0].intensity, 0.0f) : 0.0f;
    const sigpu_shared_material_t material = {
        .size_bloom = { cuboid.width, cuboid.height, cuboid.depth, bloom },
        .color = {
            g_sigpu.linear_lut[color.r] / 255.0f,
            g_sigpu.linear_lut[color.g] / 255.0f,
            g_sigpu.linear_lut[color.b] / 255.0f,
            color.a / 255.0f,
        },
    };
    const Uint32 first_axis = g_sigpu.shared_axis_count;
    const Uint32 first_rotated = g_sigpu.shared_rotated_count;

    for (uint32_t index = 0; index < it->count; ++index) {
        const auto &position = positions[index];
        const auto &rotation = rotations[index];
        const auto &scale = scales[index];
        const float width = cuboid.width * scale.x;
        const float height = cuboid.height * scale.y;
        const float depth = cuboid.depth * scale.z;
        if (!visible(position, width, height, depth, aspect)) {
            continue;
        }

        if (rotation.x == 0.0f && rotation.y == 0.0f && rotation.z == 0.0f) {
            if (g_sigpu.shared_axis_count + g_sigpu.owned_axis_count == g_sigpu.axis_capacity) {
                sigpu_axis_instances_grow();
            }
            auto *instances = static_cast<sigpu_shared_axis_instance_t *>(g_sigpu.axis_mapped);
            auto &instance = instances[g_sigpu.shared_axis_count++];
            instance = { position.x, position.y, position.z, scale.x, scale.y, scale.z };
            continue;
        }

        if (g_sigpu.shared_rotated_count + g_sigpu.owned_rotated_count ==
            g_sigpu.rotated_capacity) {
            sigpu_rotated_instances_grow();
        }
        auto *instances = static_cast<sigpu_shared_rotated_instance_t *>(g_sigpu.rotated_mapped);
        auto &instance = instances[g_sigpu.shared_rotated_count++];
        const auto packed = pack_rotation(rotation);
        instance = {
            position.x, position.y, position.z, scale.x,  scale.y,
            scale.z,    packed.x,   packed.y,   packed.z, packed.w,
        };
    }

    g_sigpu.any_bloom =
        g_sigpu.any_bloom || (bloom > 0.0f && (first_axis != g_sigpu.shared_axis_count ||
                                               first_rotated != g_sigpu.shared_rotated_count));

    record_batch(
        g_sigpu.shared_axis_batches,
        g_sigpu.shared_axis_batch_count,
        g_sigpu.shared_axis_batch_capacity,
        first_axis,
        g_sigpu.shared_axis_count,
        material
    );
    record_batch(
        g_sigpu.shared_rotated_batches,
        g_sigpu.shared_rotated_batch_count,
        g_sigpu.shared_rotated_batch_capacity,
        first_rotated,
        g_sigpu.shared_rotated_count,
        material
    );
}

void render_owned_cuboids(
    ecs_iter_t *it,
    table_field<const GlobalPosition3d> positions,
    table_field<const GlobalRotation3d> rotations,
    table_field<const GlobalScale3d> scales,
    table_field<const Cuboid> cuboids,
    table_field<const Color> colors,
    table_field<const Bloom> blooms,
    float aspect
) {
    for (uint32_t index = 0; index < it->count; ++index) {
        const auto &position = positions[index];
        const auto &rotation = rotations[index];
        const float width = cuboids[index].width * scales[index].x;
        const float height = cuboids[index].height * scales[index].y;
        const float depth = cuboids[index].depth * scales[index].z;
        if (!visible(position, width, height, depth, aspect)) {
            continue;
        }

        const float bloom = blooms.data ? std::fmax(blooms[index].intensity, 0.0f) : 0.0f;
        const auto &color = colors[index];
        g_sigpu.any_bloom = g_sigpu.any_bloom || bloom > 0.0f;

        if (rotation.x == 0.0f && rotation.y == 0.0f && rotation.z == 0.0f) {
            if (g_sigpu.shared_axis_count + g_sigpu.owned_axis_count == g_sigpu.axis_capacity) {
                sigpu_axis_instances_grow();
            }
            auto *instances = static_cast<sigpu_axis_instance_t *>(g_sigpu.axis_mapped);
            auto &instance = instances[g_sigpu.axis_capacity - ++g_sigpu.owned_axis_count];
            instance.x = position.x;
            instance.y = position.y;
            instance.z = position.z;
            instance.width = width;
            instance.height = height;
            instance.depth = depth;
            instance.r = g_sigpu.linear_lut[color.r];
            instance.g = g_sigpu.linear_lut[color.g];
            instance.b = g_sigpu.linear_lut[color.b];
            instance.a = color.a;
            instance.bloom = bloom;
            continue;
        }

        if (g_sigpu.shared_rotated_count + g_sigpu.owned_rotated_count ==
            g_sigpu.rotated_capacity) {
            sigpu_rotated_instances_grow();
        }
        auto *instances = static_cast<sigpu_rotated_instance_t *>(g_sigpu.rotated_mapped);
        auto &instance = instances[g_sigpu.rotated_capacity - ++g_sigpu.owned_rotated_count];
        const auto packed = pack_rotation(rotation);
        instance.x = position.x;
        instance.y = position.y;
        instance.z = position.z;
        instance.width = width;
        instance.height = height;
        instance.depth = depth;
        instance.qx = packed.x;
        instance.qy = packed.y;
        instance.qz = packed.z;
        instance.qw = packed.w;
        instance.r = g_sigpu.linear_lut[color.r];
        instance.g = g_sigpu.linear_lut[color.g];
        instance.b = g_sigpu.linear_lut[color.b];
        instance.a = color.a;
        instance.bloom = bloom;
    }
}

void render_cuboids(ecs_iter_t *it) {
    const auto positions = field<const GlobalPosition3d>(it, 0);
    const auto rotations = field<const GlobalRotation3d>(it, 1);
    const auto scales = field<const GlobalScale3d>(it, 2);
    const auto cuboids = field<const Cuboid>(it, 3);
    const auto colors = field<const Color>(it, 4);
    const auto blooms = field<const Bloom>(it, 5);
    const float aspect =
        static_cast<float>(g_sigpu.frame_width) / static_cast<float>(g_sigpu.frame_height);
    const bool shared = ecs_field_is_shared(it, 3) && ecs_field_is_shared(it, 4) &&
                        (!blooms.data || ecs_field_is_shared(it, 5));

    if (shared) {
        render_shared_cuboids(it, positions, rotations, scales, cuboids, colors, blooms, aspect);
    } else {
        render_owned_cuboids(it, positions, rotations, scales, cuboids, colors, blooms, aspect);
    }
}

void register_render_cuboids() {
    ecs_system_desc_t system = {
        .name = "RenderCuboids",
        .query = {
            .components = {
                { .id = ecs::detail::ecs_cpp_component_id<GlobalPosition3d>(), .access = EcsIn },
                { .id = ecs::detail::ecs_cpp_component_id<GlobalRotation3d>(), .access = EcsIn },
                { .id = ecs::detail::ecs_cpp_component_id<GlobalScale3d>(), .access = EcsIn },
                { .id = ecs::detail::ecs_cpp_component_id<Cuboid>(), .access = EcsIn },
                { .id = ecs::detail::ecs_cpp_component_id<Color>(), .access = EcsIn },
                { .id = ecs::detail::ecs_cpp_component_id<Bloom>(), .access = EcsInOptional },
            },
        },
        .callback = render_cuboids,
        .phase = EcsOnRender,
        .main_thread_only = true,
    };
    ecs_system_init(&system);
}

void begin_shadow_bounds(ecs_iter_t *) {
    if (g_sigpu.shadows_enabled) {
        sigpu_shadow_bounds_begin(
            static_cast<float>(g_sigpu.frame_width) / static_cast<float>(g_sigpu.frame_height)
        );
    }
}

void build_shadow_bounds(ecs_iter_t *it) {
    if (!g_sigpu.shadows_enabled) {
        return;
    }

    const auto positions = field<const GlobalPosition3d>(it, 0);
    const auto scales = field<const GlobalScale3d>(it, 1);
    const auto cuboids = field<const Cuboid>(it, 2);
    for (uint32_t index = 0; index < it->count; ++index) {
        const float width = cuboids[index].width * scales[index].x;
        const float height = cuboids[index].height * scales[index].y;
        const float depth = cuboids[index].depth * scales[index].z;
        sigpu_shadow_bounds_extend(
            { positions[index].x, positions[index].y, positions[index].z },
            0.5f * std::sqrt(width * width + height * height + depth * depth)
        );
    }
}

void end_shadow_bounds(ecs_iter_t *) {
    if (g_sigpu.shadows_enabled) {
        sigpu_shadow_bounds_end();
    }
}

void register_shadow_bounds(ecs_system_id_t camera_system) {
    ecs_system_desc_t begin = {
        .name = "BeginShadowBounds",
        .callback = begin_shadow_bounds,
        .phase = EcsPreRender,
        .after = { camera_system },
        .main_thread_only = true,
    };
    const ecs_system_id_t begin_system = ecs_system_init(&begin);
    ecs_system_desc_t build = {
        .name = "BuildShadowBounds",
        .query = {
            .components = {
                { .id = ecs::detail::ecs_cpp_component_id<GlobalPosition3d>(), .access = EcsIn },
                { .id = ecs::detail::ecs_cpp_component_id<GlobalScale3d>(), .access = EcsIn },
                { .id = ecs::detail::ecs_cpp_component_id<Cuboid>(), .access = EcsIn },
            },
        },
        .callback = build_shadow_bounds,
        .phase = EcsPreRender,
        .after = { begin_system },
        .main_thread_only = true,
    };
    const ecs_system_id_t build_system = ecs_system_init(&build);
    ecs_system_desc_t end = {
        .name = "EndShadowBounds",
        .callback = end_shadow_bounds,
        .phase = EcsPreRender,
        .after = { build_system },
        .main_thread_only = true,
    };
    ecs_system_init(&end);
}

void set_sky(const Sky &sky) { sigpu_sky(to_sigpu(sky.color)); }

void set_sun(const Sun &sun) { sigpu_sun(sun.x, sun.y, sun.z, to_sigpu(sun.color), sun.intensity); }

void set_ambient(const AmbientLight &ambient) {
    sigpu_ambient(to_sigpu(ambient.color), ambient.intensity);
}

void set_fog(const Fog &fog) { sigpu_fog(to_sigpu(fog.color), fog.start, fog.end); }

void set_shadows(const Shadows &shadows) { sigpu_shadows(shadows.enabled, shadows.distance); }

void set_multisampling(const Multisampling &multisampling) { sigpu_msaa(multisampling.samples); }

void set_bloom(const BloomSettings &bloom) {
    sigpu_bloom(bloom.enabled, bloom.threshold, bloom.intensity);
}

} // namespace

Color Color::yellow() { return Color{ 255, 255, 0, 255 }; }
Color Color::green() { return Color{ 0, 255, 0, 255 }; }
Color Color::red() { return Color{ 255, 0, 0, 255 }; }
Color Color::blue() { return Color{ 0, 0, 255, 255 }; }
Color Color::lblue() { return Color{ 100, 100, 255, 255 }; }
Color Color::brown() { return Color{ 139, 69, 19, 255 }; }
Color Color::gray() { return Color{ 128, 128, 128, 255 }; }

Cuboid Cuboid::splat(float value) { return Cuboid{ value, value, value }; }

bool Keyboard::down(Key key) const { return keys[static_cast<std::size_t>(key)]; }

void rendering::import() {
    sireflect_register_enum(&engine_key_reflection);

    ecs::import<sispatial>();
    ecs::component<Color>(ecs::component_options<Color>{
        .inheritance = EcsInheritShared,
    });
    ecs::component<Cuboid>(ecs::component_options<Cuboid>{
        .inheritance = EcsInheritShared,
    });
    ecs::component<Bloom>(ecs::component_options<Bloom>{
        .inheritance = EcsInheritShared,
    });
    ecs::component<Camera>();

    ecs::set_resource(
        WindowConfig{
            .width = 1280,
            .height = 800,
            .title = "R-Type",
        }
    );
    const auto &window = ecs::resource<const WindowConfig>();
    sigpu_init(window.title, window.width, window.height, default_multisampling);

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
    ecs::resource_handle<Multisampling>({ .on_set = set_multisampling })
        .set(Multisampling{ default_multisampling });
    ecs::resource_handle<BloomSettings>({ .on_set = set_bloom })
        .set(BloomSettings{ true, 0.0f, 1.0f });
    ecs::set_resource(Keyboard{});

    ecs::system("BeginRendering")
        .phase(EcsPreUpdate)
        .immediate()
        .each([](ecs::res<Keyboard> keyboard) {
            if (!sigpu_begin_frame()) {
                ecs::quit();
            }

            constexpr std::array scancodes{
                SDL_SCANCODE_A,     SDL_SCANCODE_D,  SDL_SCANCODE_W,    SDL_SCANCODE_S,
                SDL_SCANCODE_Q,     SDL_SCANCODE_Z,  SDL_SCANCODE_E,    SDL_SCANCODE_LEFT,
                SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_SPACE,
            };
            const bool *state = SDL_GetKeyboardState(nullptr);
            for (std::size_t index = 0; index < scancodes.size(); ++index) {
                keyboard->keys[index] = state[scancodes[index]];
            }
        });

    const ecs_system_id_t update_camera =
        ecs::system("UpdateCamera")
            .phase(EcsPreRender)
            .immediate()
            .each([](const GlobalPosition3d &position,
                     const GlobalRotation3d &rotation,
                     const Camera &camera) {
                const float sin_x = std::sin(rotation.x);
                const float cos_x = std::cos(rotation.x);
                const float sin_y = std::sin(rotation.y);
                const float cos_y = std::cos(rotation.y);
                const float sin_z = std::sin(rotation.z);
                const float cos_z = std::cos(rotation.z);
                const float forward_x = cos_z * sin_y * cos_x + sin_z * sin_x;
                const float forward_y = sin_z * sin_y * cos_x - cos_z * sin_x;
                const float forward_z = cos_y * cos_x;

                sigpu_camera(
                    position.x,
                    position.y,
                    position.z,
                    position.x + forward_x,
                    position.y + forward_y,
                    position.z + forward_z,
                    camera.fov
                );
                sigpu_view_prepare(
                    static_cast<float>(g_sigpu.frame_width) /
                    static_cast<float>(g_sigpu.frame_height)
                );
            });

    register_shadow_bounds(update_camera);
    register_render_cuboids();

    ecs::system("EndRendering").phase(EcsPostRender).immediate().each([] { sigpu_end_frame(); });
    ecs::at_fini([] { sigpu_fini(); });
}

} // namespace engine
