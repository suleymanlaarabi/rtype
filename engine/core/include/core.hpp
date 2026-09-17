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
    static Timer seconds(float seconds);
};

struct DisableFor {
    Timer timer;

    static DisableFor seconds(float seconds);
};

struct DespawnIn {
    Timer timer;

    static DespawnIn seconds(float seconds);
};

} // namespace engine
