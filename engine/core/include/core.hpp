#pragma once
#include <flecs.h>
#include <string>

struct engine_module {
    explicit engine_module(flecs::world &);
};

namespace engine {

using module = engine_module;

struct WindowConfig {
    int width{ 800 };
    int height{ 600 };
    std::string title{ "R-Type" };
};

struct Circle {
    float radius = 0;
};

struct Rectangle {
    float width;
    float height;
};

struct Timer {
    float elapsed;
    float duration;

    bool tick(float delta);
    static Timer from_seconds(float seconds);
};

struct DespawnIn {
    Timer timer;

    static DespawnIn from_seconds(float seconds);
};

} // namespace engine
