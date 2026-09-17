#ifndef SIGPU_H
#define SIGPU_H

#include "sigpu/bake_config.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} sigpu_color_t;

#define sigpu_rgba(r, g, b, a) ((sigpu_color_t){ r, g, b, a })
#define sigpu_rgb(r, g, b) sigpu_rgba(r, g, b, 255)

void sigpu_init(const char *title, int width, int height);
void sigpu_fini(void);

bool sigpu_begin_frame(void);
void sigpu_end_frame(void);

void sigpu_camera(
    float x,
    float y,
    float z,
    float target_x,
    float target_y,
    float target_z,
    float fov
);

void sigpu_sky(sigpu_color_t color);
void sigpu_sun(float dx, float dy, float dz, sigpu_color_t color, float intensity);
void sigpu_ambient(sigpu_color_t color, float intensity);
void sigpu_fog(sigpu_color_t color, float start, float end);
void sigpu_shadows(bool enabled);
void sigpu_shadow_distance(float distance);
void sigpu_msaa(int samples);

void sigpu_cube(
    float x,
    float y,
    float z,
    float width,
    float height,
    float depth,
    sigpu_color_t color
);

void sigpu_cube_rotated(
    float x,
    float y,
    float z,
    float width,
    float height,
    float depth,
    float rx,
    float ry,
    float rz,
    sigpu_color_t color
);

#ifdef __cplusplus
}
#endif

#endif
