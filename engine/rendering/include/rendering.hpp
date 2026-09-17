#pragma once

#include <siecs.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace engine {

struct Color {
    reflected(uint8_t r; uint8_t g; uint8_t b; uint8_t a;);

    static Color green();
    static Color red();
    static Color blue();
    static Color lblue();
    static Color brown();
    static Color yellow();
    static Color gray();
};

struct Cuboid {
    reflected(float width; float height; float depth;);

    static Cuboid splat(float value);
};

struct Bloom {
    reflected(float intensity;);
};

struct WindowConfig {
    int width;
    int height;
    const char *title;
};

struct Camera {
    reflected(float fov;);
};

struct Sky {
    Color color;
};

struct Sun {
    float x;
    float y;
    float z;
    Color color;
    float intensity;
};

struct AmbientLight {
    Color color;
    float intensity;
};

struct Fog {
    Color color;
    float start;
    float end;
};

struct Shadows {
    bool enabled;
    float distance;
};

struct Multisampling {
    int samples;
};

struct BloomSettings {
    bool enabled;
    float threshold;
    float intensity;
};

enum class Key : uint8_t {
    A,
    D,
    W,
    S,
    Q,
    Z,
    E,
    Left,
    Right,
    Up,
    Down,
    Space,
    Count,
};

struct Keyboard {
    std::array<bool, static_cast<std::size_t>(Key::Count)> keys{};

    bool down(Key key) const;
};

struct rendering {
    static void import();
};

} // namespace engine
