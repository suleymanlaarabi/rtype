#pragma once

#include <siecs.h>

namespace engine {

#ifndef NDEBUG

struct DebugTransform {};

struct debug {
    static void import();
};

#endif

} // namespace engine
