#pragma once
#include <siecs.h>

namespace engine {

#define vec2(cname)                                                                                \
    struct cname {                                                                                 \
        reflected(float x; float y;) static cname zero() { return { 0, 0 }; }                      \
    }

vec2(Position);
vec2(Velocity);

struct spatial {
    static void import();
};

} // namespace engine
