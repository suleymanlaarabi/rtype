#pragma once
#include <siecs.h>

namespace engine {

struct core {
    static void import();
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
