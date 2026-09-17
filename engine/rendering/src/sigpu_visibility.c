#include "sigpu_internal.h"

static void camera_corners(float aspect, float far_distance, sigpu_vec3_t corners[8]) {
    sigpu_vec3_t forward =
        sigpu_vec3_normalize(sigpu_vec3_sub(g_sigpu.camera.target, g_sigpu.camera.position));
    sigpu_vec3_t right =
        sigpu_vec3_normalize(sigpu_vec3_cross((sigpu_vec3_t){ 0.0f, 1.0f, 0.0f }, forward));
    sigpu_vec3_t up = sigpu_vec3_cross(forward, right);
    float tangent = tanf(g_sigpu.camera.fov * SIGPU_PI / 360.0f);
    float near_height = tangent * g_sigpu.camera.near_plane;
    float near_width = near_height * aspect;
    float far_height = tangent * far_distance;
    float far_width = far_height * aspect;
    sigpu_vec3_t near_center = sigpu_vec3_add(
        g_sigpu.camera.position,
        sigpu_vec3_scale(forward, g_sigpu.camera.near_plane)
    );
    sigpu_vec3_t far_center =
        sigpu_vec3_add(g_sigpu.camera.position, sigpu_vec3_scale(forward, far_distance));

    corners[0] = sigpu_vec3_add(
        sigpu_vec3_add(near_center, sigpu_vec3_scale(right, -near_width)),
        sigpu_vec3_scale(up, -near_height)
    );
    corners[1] = sigpu_vec3_add(
        sigpu_vec3_add(near_center, sigpu_vec3_scale(right, near_width)),
        sigpu_vec3_scale(up, -near_height)
    );
    corners[2] = sigpu_vec3_add(
        sigpu_vec3_add(near_center, sigpu_vec3_scale(right, near_width)),
        sigpu_vec3_scale(up, near_height)
    );
    corners[3] = sigpu_vec3_add(
        sigpu_vec3_add(near_center, sigpu_vec3_scale(right, -near_width)),
        sigpu_vec3_scale(up, near_height)
    );
    corners[4] = sigpu_vec3_add(
        sigpu_vec3_add(far_center, sigpu_vec3_scale(right, -far_width)),
        sigpu_vec3_scale(up, -far_height)
    );
    corners[5] = sigpu_vec3_add(
        sigpu_vec3_add(far_center, sigpu_vec3_scale(right, far_width)),
        sigpu_vec3_scale(up, -far_height)
    );
    corners[6] = sigpu_vec3_add(
        sigpu_vec3_add(far_center, sigpu_vec3_scale(right, far_width)),
        sigpu_vec3_scale(up, far_height)
    );
    corners[7] = sigpu_vec3_add(
        sigpu_vec3_add(far_center, sigpu_vec3_scale(right, -far_width)),
        sigpu_vec3_scale(up, far_height)
    );
}

static void extend_shadow_depth(sigpu_vec3_t center, float radius, float *minimum, float *maximum) {
    sigpu_vec3_t light_position = sigpu_mat4_transform_point(g_sigpu.light_view, center);

    if (light_position.x + radius >= g_sigpu.light_min_x &&
        light_position.x - radius <= g_sigpu.light_max_x &&
        light_position.y + radius >= g_sigpu.light_min_y &&
        light_position.y - radius <= g_sigpu.light_max_y) {
        *minimum = fminf(*minimum, light_position.z - radius);
        *maximum = fmaxf(*maximum, light_position.z + radius);
    }
}

static void build_shadow_transform(float aspect) {
    float shadow_distance = fminf(g_sigpu.camera.far_plane, g_sigpu.shadow_distance);
    sigpu_vec3_t corners[8];
    camera_corners(aspect, shadow_distance, corners);
    sigpu_vec3_t center = { 0.0f, 0.0f, 0.0f };

    for (int index = 0; index < 8; index++) {
        center = sigpu_vec3_add(center, sigpu_vec3_scale(corners[index], 0.125f));
    }

    sigpu_vec3_t up = fabsf(g_sigpu.sun_direction.y) > 0.99f ? (sigpu_vec3_t){ 1.0f, 0.0f, 0.0f }
                                                             : (sigpu_vec3_t){ 0.0f, 1.0f, 0.0f };
    g_sigpu.light_view =
        sigpu_mat4_look_at_lh(center, sigpu_vec3_add(center, g_sigpu.sun_direction), up);
    g_sigpu.light_min_x = INFINITY;
    g_sigpu.light_max_x = -INFINITY;
    g_sigpu.light_min_y = INFINITY;
    g_sigpu.light_max_y = -INFINITY;
    float minimum_z = INFINITY;
    float maximum_z = -INFINITY;

    for (int index = 0; index < 8; index++) {
        sigpu_vec3_t point = sigpu_mat4_transform_point(g_sigpu.light_view, corners[index]);
        g_sigpu.light_min_x = fminf(g_sigpu.light_min_x, point.x);
        g_sigpu.light_max_x = fmaxf(g_sigpu.light_max_x, point.x);
        g_sigpu.light_min_y = fminf(g_sigpu.light_min_y, point.y);
        g_sigpu.light_max_y = fmaxf(g_sigpu.light_max_y, point.y);
        minimum_z = fminf(minimum_z, point.z);
        maximum_z = fmaxf(maximum_z, point.z);
    }

    g_sigpu.light_min_x -= 1.0f;
    g_sigpu.light_max_x += 1.0f;
    g_sigpu.light_min_y -= 1.0f;
    g_sigpu.light_max_y += 1.0f;

    for (Uint32 index = 0; index < g_sigpu.axis_count; index++) {
        sigpu_axis_instance_t *cube = &g_sigpu.axis_instances[index];
        extend_shadow_depth(
            (sigpu_vec3_t){ cube->x, cube->y, cube->z },
            sigpu_cube_radius(cube->width, cube->height, cube->depth),
            &minimum_z,
            &maximum_z
        );
    }

    for (Uint32 index = 0; index < g_sigpu.rotated_count; index++) {
        sigpu_rotated_instance_t *cube = &g_sigpu.rotated_instances[index];
        extend_shadow_depth(
            (sigpu_vec3_t){ cube->x, cube->y, cube->z },
            sigpu_cube_radius(cube->width, cube->height, cube->depth),
            &minimum_z,
            &maximum_z
        );
    }

    float width = g_sigpu.light_max_x - g_sigpu.light_min_x;
    float height = g_sigpu.light_max_y - g_sigpu.light_min_y;
    float center_x = (g_sigpu.light_min_x + g_sigpu.light_max_x) * 0.5f;
    float center_y = (g_sigpu.light_min_y + g_sigpu.light_max_y) * 0.5f;
    center_x = roundf(center_x / (width / SIGPU_SHADOW_SIZE)) * (width / SIGPU_SHADOW_SIZE);
    center_y = roundf(center_y / (height / SIGPU_SHADOW_SIZE)) * (height / SIGPU_SHADOW_SIZE);
    g_sigpu.light_min_x = center_x - width * 0.5f;
    g_sigpu.light_max_x = center_x + width * 0.5f;
    g_sigpu.light_min_y = center_y - height * 0.5f;
    g_sigpu.light_max_y = center_y + height * 0.5f;

    float depth_shift = minimum_z - 5.0f;
    sigpu_vec3_t light_eye =
        sigpu_vec3_add(center, sigpu_vec3_scale(g_sigpu.sun_direction, depth_shift));
    g_sigpu.light_view =
        sigpu_mat4_look_at_lh(light_eye, sigpu_vec3_add(light_eye, g_sigpu.sun_direction), up);
    g_sigpu.light_near = 1.0f;
    g_sigpu.light_far = maximum_z - minimum_z + 10.0f;
    sigpu_mat4_t projection = sigpu_mat4_orthographic_lh(
        g_sigpu.light_min_x,
        g_sigpu.light_max_x,
        g_sigpu.light_min_y,
        g_sigpu.light_max_y,
        g_sigpu.light_near,
        g_sigpu.light_far
    );
    g_sigpu.light_view_projection = sigpu_mat4_mul(projection, g_sigpu.light_view);
}

static bool camera_visible(sigpu_vec3_t center, float radius, float aspect) {
    sigpu_vec3_t position = sigpu_mat4_transform_point(g_sigpu.view, center);

    if (position.z + radius < g_sigpu.camera.near_plane ||
        position.z - radius > g_sigpu.camera.far_plane) {
        return false;
    }

    float extent_y =
        fmaxf(position.z, g_sigpu.camera.near_plane) * tanf(g_sigpu.camera.fov * SIGPU_PI / 360.0f);
    float extent_x = extent_y * aspect;
    return fabsf(position.x) <= extent_x + radius && fabsf(position.y) <= extent_y + radius;
}

static bool shadow_visible(sigpu_vec3_t center, float radius) {
    sigpu_vec3_t position = sigpu_mat4_transform_point(g_sigpu.light_view, center);
    return position.x + radius >= g_sigpu.light_min_x &&
           position.x - radius <= g_sigpu.light_max_x &&
           position.y + radius >= g_sigpu.light_min_y &&
           position.y - radius <= g_sigpu.light_max_y &&
           position.z + radius >= g_sigpu.light_near && position.z - radius <= g_sigpu.light_far;
}

static void compact_instances(float aspect) {
    Uint32 output = 0;

    for (Uint32 index = 0; index < g_sigpu.axis_count; index++) {
        sigpu_axis_instance_t cube = g_sigpu.axis_instances[index];
        sigpu_vec3_t center = { cube.x, cube.y, cube.z };
        float radius = sigpu_cube_radius(cube.width, cube.height, cube.depth);

        if (camera_visible(center, radius, aspect) ||
            (g_sigpu.shadows_enabled && shadow_visible(center, radius))) {
            g_sigpu.axis_instances[output++] = cube;
        }
    }

    g_sigpu.axis_count = output;
    output = 0;

    for (Uint32 index = 0; index < g_sigpu.rotated_count; index++) {
        sigpu_rotated_instance_t cube = g_sigpu.rotated_instances[index];
        sigpu_vec3_t center = { cube.x, cube.y, cube.z };
        float radius = sigpu_cube_radius(cube.width, cube.height, cube.depth);

        if (camera_visible(center, radius, aspect) ||
            (g_sigpu.shadows_enabled && shadow_visible(center, radius))) {
            g_sigpu.rotated_instances[output++] = cube;
        }
    }

    g_sigpu.rotated_count = output;
}

void sigpu_visibility_prepare(float aspect) {
    g_sigpu.view = sigpu_mat4_look_at_lh(
        g_sigpu.camera.position,
        g_sigpu.camera.target,
        (sigpu_vec3_t){ 0.0f, 1.0f, 0.0f }
    );
    sigpu_mat4_t projection = sigpu_mat4_perspective_lh(
        g_sigpu.camera.fov,
        aspect,
        g_sigpu.camera.near_plane,
        g_sigpu.camera.far_plane
    );
    g_sigpu.view_projection = sigpu_mat4_mul(projection, g_sigpu.view);

    if (g_sigpu.shadows_enabled) {
        build_shadow_transform(aspect);
    } else {
        g_sigpu.light_view_projection = sigpu_mat4_identity();
    }

    compact_instances(aspect);
}
