#pragma once

#include <siecs_spatial.h>

#include <algorithm>
#include <cmath>

namespace simulation::math {

inline float length_squared(float x, float y, float z) { return x * x + y * y + z * z; }

inline float length(float x, float y, float z) { return std::sqrt(length_squared(x, y, z)); }

inline Direction3d normalize(float x, float y, float z) {
    const float inverse_length = 1.0f / length(x, y, z);
    return { x * inverse_length, y * inverse_length, z * inverse_length };
}

inline Direction3d forward(const Rotation3d &rotation) {
    const float cos_pitch = std::cos(rotation.pitch);
    return {
        std::sin(rotation.yaw) * cos_pitch,
        -std::sin(rotation.pitch),
        std::cos(rotation.yaw) * cos_pitch,
    };
}

inline float wrap_angle(float angle) {
    constexpr float pi = 3.14159265358979323846f;
    constexpr float tau = pi * 2.0f;
    while (angle > pi) {
        angle -= tau;
    }
    while (angle < -pi) {
        angle += tau;
    }
    return angle;
}

inline float approach(float current, float target, float maximum_delta) {
    return current + std::clamp(target - current, -maximum_delta, maximum_delta);
}

inline float approach_angle(float current, float target, float maximum_delta) {
    return current + std::clamp(wrap_angle(target - current), -maximum_delta, maximum_delta);
}

} // namespace simulation::math
