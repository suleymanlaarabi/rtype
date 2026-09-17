#include "sigpu.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SIGPU_AXIS_CAPACITY 131072
#define SIGPU_ROTATED_CAPACITY 16384
#define SIGPU_SHADOW_SIZE 2048
#define SIGPU_HDR_FORMAT SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT
#define SIGPU_PI 3.14159265358979323846f
#define SIGPU_SHADER(name) SIGPU_SHADER_DIR "/" name

typedef struct {
    float x;
    float y;
    float z;
} vec3_t;

typedef struct {
    float m[16];
} mat4_t;

typedef struct {
    float x;
    float y;
    float z;
    int8_t nx;
    int8_t ny;
    int8_t nz;
    int8_t nw;
} cubevertex_t;

typedef struct {
    float x;
    float y;
    float z;
    float width;
    float height;
    float depth;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
    float bloom;
} axisinstance_t;

typedef struct {
    float x;
    float y;
    float z;
    float width;
    float height;
    float depth;
    int16_t qx;
    int16_t qy;
    int16_t qz;
    int16_t qw;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
    float bloom;
} sirotated_instance_t;

typedef struct {
    vec3_t position;
    vec3_t target;
    float fov;
    float near_plane;
    float far_plane;
} camera_t;

typedef struct {
    mat4_t view_projection;
    mat4_t light_view_projection;
} transform_uniform_t;

typedef struct {
    float camera_position[4];
    float sun_direction_intensity[4];
    float sun_color[4];
    float ambient_color_intensity[4];
    float fog_color[4];
    float fog_parameters[4];
    float shadow_parameters[4];
} lighting_uniform_t;

static SDL_Window *g_window;
static SDL_GPUDevice *g_device;
static SDL_GPUCommandBuffer *g_command_buffer;
static SDL_GPUTexture *g_swapchain;
static SDL_GPUTextureFormat g_swapchain_format;
static bool g_window_claimed;
static bool g_linear_swapchain;

static SDL_GPUGraphicsPipeline *g_axis_pipeline;
static SDL_GPUGraphicsPipeline *g_rotated_pipeline;
static SDL_GPUGraphicsPipeline *g_axis_shadow_pipeline;
static SDL_GPUGraphicsPipeline *g_rotated_shadow_pipeline;
static SDL_GPUGraphicsPipeline *g_bloom_down_pipeline;
static SDL_GPUGraphicsPipeline *g_bloom_blur_pipeline;
static SDL_GPUGraphicsPipeline *g_bloom_composite_pipeline;

static SDL_GPUBuffer *g_vertex_buffer;
static SDL_GPUBuffer *g_index_buffer;
static SDL_GPUBuffer *g_axis_buffer;
static SDL_GPUBuffer *g_rotated_buffer;
static SDL_GPUTransferBuffer *g_axis_transfer;
static SDL_GPUTransferBuffer *g_rotated_transfer;

static SDL_GPUTexture *g_depth_texture;
static SDL_GPUTexture *g_msaa_texture;
static SDL_GPUTexture *g_bloom_msaa_texture;
static SDL_GPUTexture *g_scene_texture;
static SDL_GPUTexture *g_bloom_texture;
static SDL_GPUTexture *g_bloom_half;
static SDL_GPUTexture *g_bloom_half_scratch;
static SDL_GPUTexture *g_bloom_quarter;
static SDL_GPUTexture *g_bloom_quarter_scratch;
static SDL_GPUTexture *g_shadow_texture;
static SDL_GPUSampler *g_shadow_sampler;
static SDL_GPUSampler *g_bloom_sampler;

static axisinstance_t *g_axis_instances;
static sirotated_instance_t *g_rotated_instances;
static Uint32 g_axis_count;
static Uint32 g_rotated_count;
static Uint32 g_axis_capacity;
static Uint32 g_rotated_capacity;

static Uint32 g_frame_width;
static Uint32 g_frame_height;
static Uint32 g_target_width;
static Uint32 g_target_height;
static Uint32 g_bloom_half_width;
static Uint32 g_bloom_half_height;
static Uint32 g_bloom_quarter_width;
static Uint32 g_bloom_quarter_height;
static SDL_GPUSampleCount g_sample_count;

static camera_t g_camera;
static vec3_t g_sun_direction;
static SDL_FColor g_sun_color;
static SDL_FColor g_ambient_color;
static SDL_FColor g_fog_color;
static SDL_FColor g_sky_linear;
static SDL_FColor g_sky_srgb;
static float g_sun_intensity;
static float g_ambient_intensity;
static float g_fog_start;
static float g_fog_end;
static float g_shadow_distance;
static float g_bloom_threshold;
static float g_bloom_intensity;
static bool g_fog_enabled;
static bool g_shadows_enabled;
static bool g_bloom_enabled;
static bool g_any_bloom;

static uint8_t g_linear_lut[256];
static mat4_t g_view;
static mat4_t g_view_projection;
static mat4_t g_light_view;
static mat4_t g_light_view_projection;
static float g_light_min_x;
static float g_light_max_x;
static float g_light_min_y;
static float g_light_max_y;
static float g_light_near;
static float g_light_far;

static const cubevertex_t cube_vertices[24] = {
    { -0.5f, -0.5f, -0.5f, 0, 0, -127, 0 }, { 0.5f, -0.5f, -0.5f, 0, 0, -127, 0 },
    { 0.5f, 0.5f, -0.5f, 0, 0, -127, 0 },   { -0.5f, 0.5f, -0.5f, 0, 0, -127, 0 },
    { 0.5f, -0.5f, 0.5f, 0, 0, 127, 0 },    { -0.5f, -0.5f, 0.5f, 0, 0, 127, 0 },
    { -0.5f, 0.5f, 0.5f, 0, 0, 127, 0 },    { 0.5f, 0.5f, 0.5f, 0, 0, 127, 0 },
    { -0.5f, -0.5f, 0.5f, -127, 0, 0, 0 },  { -0.5f, -0.5f, -0.5f, -127, 0, 0, 0 },
    { -0.5f, 0.5f, -0.5f, -127, 0, 0, 0 },  { -0.5f, 0.5f, 0.5f, -127, 0, 0, 0 },
    { 0.5f, -0.5f, -0.5f, 127, 0, 0, 0 },   { 0.5f, -0.5f, 0.5f, 127, 0, 0, 0 },
    { 0.5f, 0.5f, 0.5f, 127, 0, 0, 0 },     { 0.5f, 0.5f, -0.5f, 127, 0, 0, 0 },
    { -0.5f, 0.5f, -0.5f, 0, 127, 0, 0 },   { 0.5f, 0.5f, -0.5f, 0, 127, 0, 0 },
    { 0.5f, 0.5f, 0.5f, 0, 127, 0, 0 },     { -0.5f, 0.5f, 0.5f, 0, 127, 0, 0 },
    { -0.5f, -0.5f, 0.5f, 0, -127, 0, 0 },  { 0.5f, -0.5f, 0.5f, 0, -127, 0, 0 },
    { 0.5f, -0.5f, -0.5f, 0, -127, 0, 0 },  { -0.5f, -0.5f, -0.5f, 0, -127, 0, 0 }
};

static const Uint16 cube_indices[36] = { 0,  1,  2,  2,  3,  0,  4,  5,  6,  6,  7,  4,
                                         8,  9,  10, 10, 11, 8,  12, 13, 14, 14, 15, 12,
                                         16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20 };

static inline vec3_t vec3_add(vec3_t a, vec3_t b) {
    return (vec3_t){ a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline vec3_t vec3_sub(vec3_t a, vec3_t b) {
    return (vec3_t){ a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline vec3_t vec3_scale(vec3_t value, float scale) {
    return (vec3_t){ value.x * scale, value.y * scale, value.z * scale };
}

static inline float vec3_dot(vec3_t a, vec3_t b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

static inline vec3_t vec3_cross(vec3_t a, vec3_t b) {
    return (vec3_t){ a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

static inline vec3_t vec3_normalize(vec3_t value) {
    float inverse_length = 1.0f / sqrtf(vec3_dot(value, value));
    return vec3_scale(value, inverse_length);
}

static mat4_t mat4_identity(void) {
    mat4_t result = { 0 };
    result.m[0] = 1.0f;
    result.m[5] = 1.0f;
    result.m[10] = 1.0f;
    result.m[15] = 1.0f;
    return result;
}

static mat4_t mat4_mul(mat4_t a, mat4_t b) {
    mat4_t result = { 0 };

    for (int column = 0; column < 4; column++) {
        for (int row = 0; row < 4; row++) {
            result.m[column * 4 + row] =
                a.m[row] * b.m[column * 4] + a.m[4 + row] * b.m[column * 4 + 1] +
                a.m[8 + row] * b.m[column * 4 + 2] + a.m[12 + row] * b.m[column * 4 + 3];
        }
    }

    return result;
}

static mat4_t mat4_perspective_lh(float fov, float aspect, float near_plane, float far_plane) {
    mat4_t result = { 0 };
    float y_scale = 1.0f / tanf(fov * SIGPU_PI / 360.0f);
    float x_scale = y_scale / aspect;

    result.m[0] = x_scale;
    result.m[5] = y_scale;
    result.m[10] = far_plane / (far_plane - near_plane);
    result.m[11] = 1.0f;
    result.m[14] = -(near_plane * far_plane) / (far_plane - near_plane);

    return result;
}

static mat4_t mat4_orthographic_lh(
    float left,
    float right,
    float bottom,
    float top,
    float near_plane,
    float far_plane
) {
    mat4_t result = { 0 };

    result.m[0] = 2.0f / (right - left);
    result.m[5] = 2.0f / (top - bottom);
    result.m[10] = 1.0f / (far_plane - near_plane);
    result.m[12] = -(right + left) / (right - left);
    result.m[13] = -(top + bottom) / (top - bottom);
    result.m[14] = -near_plane / (far_plane - near_plane);
    result.m[15] = 1.0f;

    return result;
}

static mat4_t mat4_look_at_lh(vec3_t eye, vec3_t target, vec3_t up) {
    vec3_t forward = vec3_normalize(vec3_sub(target, eye));
    vec3_t right = vec3_normalize(vec3_cross(up, forward));
    vec3_t camera_up = vec3_cross(forward, right);
    mat4_t result = { 0 };

    result.m[0] = right.x;
    result.m[1] = camera_up.x;
    result.m[2] = forward.x;
    result.m[4] = right.y;
    result.m[5] = camera_up.y;
    result.m[6] = forward.y;
    result.m[8] = right.z;
    result.m[9] = camera_up.z;
    result.m[10] = forward.z;
    result.m[12] = -vec3_dot(right, eye);
    result.m[13] = -vec3_dot(camera_up, eye);
    result.m[14] = -vec3_dot(forward, eye);
    result.m[15] = 1.0f;

    return result;
}

static vec3_t mat4_transform_point(mat4_t matrix, vec3_t point) {
    return (vec3_t){
        matrix.m[0] * point.x + matrix.m[4] * point.y + matrix.m[8] * point.z + matrix.m[12],
        matrix.m[1] * point.x + matrix.m[5] * point.y + matrix.m[9] * point.z + matrix.m[13],
        matrix.m[2] * point.x + matrix.m[6] * point.y + matrix.m[10] * point.z + matrix.m[14]
    };
}

static float cube_radius(float width, float height, float depth) {
    return 0.5f * sqrtf(width * width + height * height + depth * depth);
}

static SDL_FColor linear_color(sigpu_color_t color) {
    return (SDL_FColor){ g_linear_lut[color.r] / 255.0f,
                         g_linear_lut[color.g] / 255.0f,
                         g_linear_lut[color.b] / 255.0f,
                         color.a / 255.0f };
}

static void pack_color(uint8_t *target, sigpu_color_t color) {
    target[0] = g_linear_lut[color.r];
    target[1] = g_linear_lut[color.g];
    target[2] = g_linear_lut[color.b];
    target[3] = color.a;
}

static SDL_GPUShader *load_shader(
    const char *path,
    SDL_GPUShaderStage stage,
    Uint32 sampler_count,
    Uint32 uniform_count
) {
    size_t size;
    void *code = SDL_LoadFile(path, &size);
    SDL_GPUShaderCreateInfo info = { .code_size = size,
                                     .code = code,
                                     .entrypoint = "main",
                                     .format = SDL_GPU_SHADERFORMAT_SPIRV,
                                     .stage = stage,
                                     .num_samplers = sampler_count,
                                     .num_uniform_buffers = uniform_count };
    SDL_GPUShader *shader = SDL_CreateGPUShader(g_device, &info);
    SDL_free(code);
    return shader;
}

static SDL_GPUGraphicsPipeline *create_main_pipeline(bool rotated) {
    SDL_GPUShader *vertex_shader = load_shader(
        rotated ? SIGPU_SHADER("cube_rotated.vert.spv") : SIGPU_SHADER("cube.vert.spv"),
        SDL_GPU_SHADERSTAGE_VERTEX,
        0,
        1
    );
    SDL_GPUShader *fragment_shader = load_shader(
        SIGPU_SHADER("cube.frag.spv"),
        SDL_GPU_SHADERSTAGE_FRAGMENT,
        1,
        1
    );
    SDL_GPUVertexBufferDescription buffers[2] = {
        {
            .slot = 0,
            .pitch = sizeof(cubevertex_t),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        },
        {
            .slot = 1,
            .pitch = rotated ? sizeof(sirotated_instance_t) : sizeof(axisinstance_t),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
        },
    };
    SDL_GPUVertexAttribute attributes[7] = {
        {
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(cubevertex_t, x),
        },
        {
            .location = 1,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_BYTE4_NORM,
            .offset = offsetof(cubevertex_t, nx),
        },
        {
            .location = 2,
            .buffer_slot = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = rotated ? offsetof(sirotated_instance_t, x) : offsetof(axisinstance_t, x),
        },
        {
            .location = 3,
            .buffer_slot = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset =
                rotated ? offsetof(sirotated_instance_t, width) : offsetof(axisinstance_t, width),
        },
        {
            .location = 4,
            .buffer_slot = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
            .offset = rotated ? offsetof(sirotated_instance_t, r) : offsetof(axisinstance_t, r),
        },
        {
            .location = 5,
            .buffer_slot = 1,
            .format = rotated ? SDL_GPU_VERTEXELEMENTFORMAT_SHORT4_NORM
                              : SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
            .offset =
                rotated ? offsetof(sirotated_instance_t, qx) : offsetof(axisinstance_t, bloom),
        },
        {
            .location = 6,
            .buffer_slot = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
            .offset = offsetof(sirotated_instance_t, bloom),
        },
    };
    SDL_GPUColorTargetDescription color_targets[2] = {
        { .format = SIGPU_HDR_FORMAT },
        { .format = SIGPU_HDR_FORMAT },
    };
    SDL_GPUGraphicsPipelineCreateInfo info = {
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .vertex_input_state = {
            .vertex_buffer_descriptions = buffers,
            .num_vertex_buffers = 2,
            .vertex_attributes = attributes,
            .num_vertex_attributes = rotated ? 7 : 6,
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .cull_mode = SDL_GPU_CULLMODE_BACK,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
            .enable_depth_clip = true,
        },
        .multisample_state = {
            .sample_count = g_sample_count,
        },
        .depth_stencil_state = {
            .compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
            .enable_depth_test = true,
            .enable_depth_write = true,
        },
        .target_info = {
            .color_target_descriptions = color_targets,
            .num_color_targets = 2,
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
            .has_depth_stencil_target = true,
        },
    };
    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(g_device, &info);
    SDL_ReleaseGPUShader(g_device, vertex_shader);
    SDL_ReleaseGPUShader(g_device, fragment_shader);
    return pipeline;
}

static SDL_GPUGraphicsPipeline *create_shadow_pipeline(bool rotated) {
    SDL_GPUShader *vertex_shader = load_shader(
        rotated ? SIGPU_SHADER("shadow_rotated.vert.spv") : SIGPU_SHADER("shadow.vert.spv"),
        SDL_GPU_SHADERSTAGE_VERTEX,
        0,
        1
    );
    SDL_GPUShader *fragment_shader =
        load_shader(SIGPU_SHADER("shadow.frag.spv"), SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0);
    SDL_GPUVertexBufferDescription buffers[2] = {
        {
            .slot = 0,
            .pitch = sizeof(cubevertex_t),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        },
        {
            .slot = 1,
            .pitch = rotated ? sizeof(sirotated_instance_t) : sizeof(axisinstance_t),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
        }
    };
    SDL_GPUVertexAttribute attributes[4] = {
        {
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(cubevertex_t, x),
        },
        {
            .location = 1,
            .buffer_slot = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = rotated ? offsetof(sirotated_instance_t, x) : offsetof(axisinstance_t, x),
        },
        {
            .location = 2,
            .buffer_slot = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset =
                rotated ? offsetof(sirotated_instance_t, width) : offsetof(axisinstance_t, width),
        },
        {
            .location = 3,
            .buffer_slot = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_SHORT4_NORM,
            .offset = offsetof(sirotated_instance_t, qx),
        }
    };
    SDL_GPUGraphicsPipelineCreateInfo info = {
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .vertex_input_state = {
            .vertex_buffer_descriptions = buffers,
            .num_vertex_buffers = 2,
            .vertex_attributes = attributes,
            .num_vertex_attributes = rotated ? 4 : 3,
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .cull_mode = SDL_GPU_CULLMODE_BACK,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
            .depth_bias_constant_factor = 1.25f,
            .depth_bias_slope_factor = 1.75f,
            .enable_depth_bias = true,
            .enable_depth_clip = true,
        },
        .multisample_state = {
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
        },
        .depth_stencil_state = {
            .compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
            .enable_depth_test = true,
            .enable_depth_write = true,
        },
        .target_info = {
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
            .has_depth_stencil_target = true,
        },
    };
    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(g_device, &info);
    SDL_ReleaseGPUShader(g_device, vertex_shader);
    SDL_ReleaseGPUShader(g_device, fragment_shader);
    return pipeline;
}

static void create_main_pipelines(void) {
    if (g_axis_pipeline) {
        SDL_ReleaseGPUGraphicsPipeline(g_device, g_axis_pipeline);
        SDL_ReleaseGPUGraphicsPipeline(g_device, g_rotated_pipeline);
    }

    g_axis_pipeline = create_main_pipeline(false);
    g_rotated_pipeline = create_main_pipeline(true);
}

static void create_shadow_resources(void) {
    g_shadow_texture = SDL_CreateGPUTexture(
        g_device,
        &(SDL_GPUTextureCreateInfo){
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
            .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
            .width = SIGPU_SHADOW_SIZE,
            .height = SIGPU_SHADOW_SIZE,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
        }
    );
    g_shadow_sampler = SDL_CreateGPUSampler(
        g_device,
        &(SDL_GPUSamplerCreateInfo){
            .min_filter = SDL_GPU_FILTER_LINEAR,
            .mag_filter = SDL_GPU_FILTER_LINEAR,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
            .enable_compare = true,
        }
    );
    g_axis_shadow_pipeline = create_shadow_pipeline(false);
    g_rotated_shadow_pipeline = create_shadow_pipeline(true);
}

static SDL_GPUGraphicsPipeline *create_fullscreen_pipeline(
    const char *fragment_path,
    Uint32 sampler_count,
    SDL_GPUTextureFormat format
) {
    SDL_GPUShader *vertex_shader =
        load_shader(SIGPU_SHADER("fullscreen.vert.spv"), SDL_GPU_SHADERSTAGE_VERTEX, 0, 0);
    SDL_GPUShader *fragment_shader =
        load_shader(fragment_path, SDL_GPU_SHADERSTAGE_FRAGMENT, sampler_count, 1);
    SDL_GPUColorTargetDescription color_target = { .format = format };
    SDL_GPUGraphicsPipelineCreateInfo info = {
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .cull_mode = SDL_GPU_CULLMODE_NONE,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        },
        .target_info = {
            .color_target_descriptions = &color_target,
            .num_color_targets = 1,
        },
    };
    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(g_device, &info);
    SDL_ReleaseGPUShader(g_device, vertex_shader);
    SDL_ReleaseGPUShader(g_device, fragment_shader);
    return pipeline;
}

static void create_bloom_resources(void) {
    g_bloom_sampler = SDL_CreateGPUSampler(
        g_device,
        &(SDL_GPUSamplerCreateInfo){
            .min_filter = SDL_GPU_FILTER_LINEAR,
            .mag_filter = SDL_GPU_FILTER_LINEAR,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        }
    );
    g_bloom_down_pipeline = create_fullscreen_pipeline(
        SIGPU_SHADER("bloom_down.frag.spv"),
        1,
        SIGPU_HDR_FORMAT
    );
    g_bloom_blur_pipeline =
        create_fullscreen_pipeline(SIGPU_SHADER("bloom_blur.frag.spv"), 1, SIGPU_HDR_FORMAT);
    g_bloom_composite_pipeline = create_fullscreen_pipeline(
        SIGPU_SHADER("bloom_composite.frag.spv"),
        3,
        g_swapchain_format
    );
}

static void create_cube_mesh(void) {
    Uint32 vertex_size = sizeof(cube_vertices);
    Uint32 index_size = sizeof(cube_indices);
    SDL_GPUTransferBuffer *transfer = SDL_CreateGPUTransferBuffer(
        g_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = vertex_size + index_size,
        }
    );
    g_vertex_buffer = SDL_CreateGPUBuffer(
        g_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = vertex_size,
        }
    );
    g_index_buffer = SDL_CreateGPUBuffer(
        g_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = index_size,
        }
    );
    uint8_t *data = SDL_MapGPUTransferBuffer(g_device, transfer, false);
    memcpy(data, cube_vertices, vertex_size);
    memcpy(data + vertex_size, cube_indices, index_size);
    SDL_UnmapGPUTransferBuffer(g_device, transfer);
    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(g_device);
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(command_buffer);
    SDL_UploadToGPUBuffer(
        copy,
        &(SDL_GPUTransferBufferLocation){ .transfer_buffer = transfer },
        &(SDL_GPUBufferRegion){ .buffer = g_vertex_buffer, .size = vertex_size },
        false
    );
    SDL_UploadToGPUBuffer(
        copy,
        &(SDL_GPUTransferBufferLocation){ .transfer_buffer = transfer, .offset = vertex_size },
        &(SDL_GPUBufferRegion){ .buffer = g_index_buffer, .size = index_size },
        false
    );
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    SDL_ReleaseGPUTransferBuffer(g_device, transfer);
}

static void resize_axis_instances(Uint32 capacity) {
    g_axis_instances = realloc(g_axis_instances, sizeof(axisinstance_t) * capacity);

    if (g_axis_buffer) {
        SDL_ReleaseGPUBuffer(g_device, g_axis_buffer);
        SDL_ReleaseGPUTransferBuffer(g_device, g_axis_transfer);
    }

    Uint32 size = sizeof(axisinstance_t) * capacity;
    g_axis_buffer = SDL_CreateGPUBuffer(
        g_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = size,
        }
    );
    g_axis_transfer = SDL_CreateGPUTransferBuffer(
        g_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = size,
        }
    );
    g_axis_capacity = capacity;
}

static void resize_rotated_instances(Uint32 capacity) {
    g_rotated_instances = realloc(g_rotated_instances, sizeof(sirotated_instance_t) * capacity);

    if (g_rotated_buffer) {
        SDL_ReleaseGPUBuffer(g_device, g_rotated_buffer);
        SDL_ReleaseGPUTransferBuffer(g_device, g_rotated_transfer);
    }

    Uint32 size = sizeof(sirotated_instance_t) * capacity;
    g_rotated_buffer = SDL_CreateGPUBuffer(
        g_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = size,
        }
    );
    g_rotated_transfer = SDL_CreateGPUTransferBuffer(
        g_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = size,
        }
    );
    g_rotated_capacity = capacity;
}

static SDL_GPUTexture *create_hdr_texture(
    Uint32 width,
    Uint32 height,
    SDL_GPUTextureUsageFlags usage,
    SDL_GPUSampleCount sample_count
) {
    return SDL_CreateGPUTexture(
        g_device,
        &(SDL_GPUTextureCreateInfo){
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SIGPU_HDR_FORMAT,
            .usage = usage,
            .width = width,
            .height = height,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = sample_count,
        }
    );
}

static void release_texture(SDL_GPUTexture **texture) {
    if (*texture) {
        SDL_ReleaseGPUTexture(g_device, *texture);
        *texture = NULL;
    }
}

static void release_frame_targets(void) {
    release_texture(&g_depth_texture);
    release_texture(&g_msaa_texture);
    release_texture(&g_bloom_msaa_texture);
    release_texture(&g_scene_texture);
    release_texture(&g_bloom_texture);
    release_texture(&g_bloom_half);
    release_texture(&g_bloom_half_scratch);
    release_texture(&g_bloom_quarter);
    release_texture(&g_bloom_quarter_scratch);

    g_target_width = 0;
    g_target_height = 0;
}

static void ensure_frame_targets(void) {
    if (g_scene_texture && g_target_width == g_frame_width && g_target_height == g_frame_height) {
        return;
    }

    release_frame_targets();
    g_depth_texture = SDL_CreateGPUTexture(
        g_device,
        &(SDL_GPUTextureCreateInfo){
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
            .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
            .width = g_frame_width,
            .height = g_frame_height,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = g_sample_count,
        }
    );
    g_scene_texture = create_hdr_texture(
        g_frame_width,
        g_frame_height,
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        SDL_GPU_SAMPLECOUNT_1
    );

    if (g_sample_count != SDL_GPU_SAMPLECOUNT_1) {
        g_msaa_texture = create_hdr_texture(
            g_frame_width,
            g_frame_height,
            SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
            g_sample_count
        );
        g_bloom_msaa_texture = create_hdr_texture(
            g_frame_width,
            g_frame_height,
            SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
            g_sample_count
        );
    }

    g_bloom_texture = create_hdr_texture(
        g_frame_width,
        g_frame_height,
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        SDL_GPU_SAMPLECOUNT_1
    );
    g_bloom_half_width = g_frame_width > 1 ? g_frame_width / 2 : 1;
    g_bloom_half_height = g_frame_height > 1 ? g_frame_height / 2 : 1;
    g_bloom_quarter_width = g_bloom_half_width > 1 ? g_bloom_half_width / 2 : 1;
    g_bloom_quarter_height = g_bloom_half_height > 1 ? g_bloom_half_height / 2 : 1;
    g_bloom_half = create_hdr_texture(
        g_bloom_half_width,
        g_bloom_half_height,
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        SDL_GPU_SAMPLECOUNT_1
    );
    g_bloom_half_scratch = create_hdr_texture(
        g_bloom_half_width,
        g_bloom_half_height,
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        SDL_GPU_SAMPLECOUNT_1
    );
    g_bloom_quarter = create_hdr_texture(
        g_bloom_quarter_width,
        g_bloom_quarter_height,
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        SDL_GPU_SAMPLECOUNT_1
    );
    g_bloom_quarter_scratch = create_hdr_texture(
        g_bloom_quarter_width,
        g_bloom_quarter_height,
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        SDL_GPU_SAMPLECOUNT_1
    );

    g_target_width = g_frame_width;
    g_target_height = g_frame_height;
}

static void camera_corners(float aspect, float far_distance, vec3_t corners[8]) {
    vec3_t forward = vec3_normalize(vec3_sub(g_camera.target, g_camera.position));
    vec3_t right = vec3_normalize(vec3_cross((vec3_t){ 0.0f, 1.0f, 0.0f }, forward));
    vec3_t up = vec3_cross(forward, right);
    float tangent = tanf(g_camera.fov * SIGPU_PI / 360.0f);
    float near_height = tangent * g_camera.near_plane;
    float near_width = near_height * aspect;
    float far_height = tangent * far_distance;
    float far_width = far_height * aspect;
    vec3_t near_center = vec3_add(g_camera.position, vec3_scale(forward, g_camera.near_plane));
    vec3_t far_center = vec3_add(g_camera.position, vec3_scale(forward, far_distance));

    corners[0] = vec3_add(
        vec3_add(near_center, vec3_scale(right, -near_width)),
        vec3_scale(up, -near_height)
    );
    corners[1] = vec3_add(
        vec3_add(near_center, vec3_scale(right, near_width)),
        vec3_scale(up, -near_height)
    );
    corners[2] =
        vec3_add(vec3_add(near_center, vec3_scale(right, near_width)), vec3_scale(up, near_height));
    corners[3] = vec3_add(
        vec3_add(near_center, vec3_scale(right, -near_width)),
        vec3_scale(up, near_height)
    );
    corners[4] =
        vec3_add(vec3_add(far_center, vec3_scale(right, -far_width)), vec3_scale(up, -far_height));
    corners[5] =
        vec3_add(vec3_add(far_center, vec3_scale(right, far_width)), vec3_scale(up, -far_height));
    corners[6] =
        vec3_add(vec3_add(far_center, vec3_scale(right, far_width)), vec3_scale(up, far_height));
    corners[7] =
        vec3_add(vec3_add(far_center, vec3_scale(right, -far_width)), vec3_scale(up, far_height));
}

static void extend_shadow_depth(vec3_t center, float radius, float *minimum, float *maximum) {
    vec3_t light_position = mat4_transform_point(g_light_view, center);

    if (light_position.x + radius >= g_light_min_x && light_position.x - radius <= g_light_max_x &&
        light_position.y + radius >= g_light_min_y && light_position.y - radius <= g_light_max_y) {
        *minimum = fminf(*minimum, light_position.z - radius);
        *maximum = fmaxf(*maximum, light_position.z + radius);
    }
}

static void build_shadow_transform(float aspect) {
    float shadow_distance = fminf(g_camera.far_plane, g_shadow_distance);
    vec3_t corners[8];
    camera_corners(aspect, shadow_distance, corners);
    vec3_t center = { 0.0f, 0.0f, 0.0f };

    for (int index = 0; index < 8; index++) {
        center = vec3_add(center, vec3_scale(corners[index], 0.125f));
    }

    vec3_t up = fabsf(g_sun_direction.y) > 0.99f ? (vec3_t){ 1.0f, 0.0f, 0.0f }
                                                 : (vec3_t){ 0.0f, 1.0f, 0.0f };
    g_light_view = mat4_look_at_lh(center, vec3_add(center, g_sun_direction), up);
    g_light_min_x = INFINITY;
    g_light_max_x = -INFINITY;
    g_light_min_y = INFINITY;
    g_light_max_y = -INFINITY;
    float minimum_z = INFINITY;
    float maximum_z = -INFINITY;

    for (int index = 0; index < 8; index++) {
        vec3_t point = mat4_transform_point(g_light_view, corners[index]);
        g_light_min_x = fminf(g_light_min_x, point.x);
        g_light_max_x = fmaxf(g_light_max_x, point.x);
        g_light_min_y = fminf(g_light_min_y, point.y);
        g_light_max_y = fmaxf(g_light_max_y, point.y);
        minimum_z = fminf(minimum_z, point.z);
        maximum_z = fmaxf(maximum_z, point.z);
    }

    g_light_min_x -= 1.0f;
    g_light_max_x += 1.0f;
    g_light_min_y -= 1.0f;
    g_light_max_y += 1.0f;

    for (Uint32 index = 0; index < g_axis_count; index++) {
        axisinstance_t *cube = &g_axis_instances[index];
        extend_shadow_depth(
            (vec3_t){ cube->x, cube->y, cube->z },
            cube_radius(cube->width, cube->height, cube->depth),
            &minimum_z,
            &maximum_z
        );
    }

    for (Uint32 index = 0; index < g_rotated_count; index++) {
        sirotated_instance_t *cube = &g_rotated_instances[index];
        extend_shadow_depth(
            (vec3_t){ cube->x, cube->y, cube->z },
            cube_radius(cube->width, cube->height, cube->depth),
            &minimum_z,
            &maximum_z
        );
    }

    float width = g_light_max_x - g_light_min_x;
    float height = g_light_max_y - g_light_min_y;
    float center_x = (g_light_min_x + g_light_max_x) * 0.5f;
    float center_y = (g_light_min_y + g_light_max_y) * 0.5f;
    center_x = roundf(center_x / (width / SIGPU_SHADOW_SIZE)) * (width / SIGPU_SHADOW_SIZE);
    center_y = roundf(center_y / (height / SIGPU_SHADOW_SIZE)) * (height / SIGPU_SHADOW_SIZE);
    g_light_min_x = center_x - width * 0.5f;
    g_light_max_x = center_x + width * 0.5f;
    g_light_min_y = center_y - height * 0.5f;
    g_light_max_y = center_y + height * 0.5f;

    float depth_shift = minimum_z - 5.0f;
    vec3_t light_eye = vec3_add(center, vec3_scale(g_sun_direction, depth_shift));
    g_light_view = mat4_look_at_lh(light_eye, vec3_add(light_eye, g_sun_direction), up);
    g_light_near = 1.0f;
    g_light_far = maximum_z - minimum_z + 10.0f;
    mat4_t projection = mat4_orthographic_lh(
        g_light_min_x,
        g_light_max_x,
        g_light_min_y,
        g_light_max_y,
        g_light_near,
        g_light_far
    );
    g_light_view_projection = mat4_mul(projection, g_light_view);
}

static bool camera_visible(vec3_t center, float radius, float aspect) {
    vec3_t position = mat4_transform_point(g_view, center);

    if (position.z + radius < g_camera.near_plane || position.z - radius > g_camera.far_plane) {
        return false;
    }

    float extent_y =
        fmaxf(position.z, g_camera.near_plane) * tanf(g_camera.fov * SIGPU_PI / 360.0f);
    float extent_x = extent_y * aspect;
    return fabsf(position.x) <= extent_x + radius && fabsf(position.y) <= extent_y + radius;
}

static bool shadow_visible(vec3_t center, float radius) {
    vec3_t position = mat4_transform_point(g_light_view, center);
    return position.x + radius >= g_light_min_x && position.x - radius <= g_light_max_x &&
           position.y + radius >= g_light_min_y && position.y - radius <= g_light_max_y &&
           position.z + radius >= g_light_near && position.z - radius <= g_light_far;
}

static void compact_instances(float aspect) {
    Uint32 output = 0;

    for (Uint32 index = 0; index < g_axis_count; index++) {
        axisinstance_t cube = g_axis_instances[index];
        vec3_t center = { cube.x, cube.y, cube.z };
        float radius = cube_radius(cube.width, cube.height, cube.depth);

        if (camera_visible(center, radius, aspect) ||
            (g_shadows_enabled && shadow_visible(center, radius))) {
            g_axis_instances[output++] = cube;
        }
    }

    g_axis_count = output;
    output = 0;

    for (Uint32 index = 0; index < g_rotated_count; index++) {
        sirotated_instance_t cube = g_rotated_instances[index];
        vec3_t center = { cube.x, cube.y, cube.z };
        float radius = cube_radius(cube.width, cube.height, cube.depth);

        if (camera_visible(center, radius, aspect) ||
            (g_shadows_enabled && shadow_visible(center, radius))) {
            g_rotated_instances[output++] = cube;
        }
    }

    g_rotated_count = output;
}

static void upload_instances(void) {
    if (g_axis_count == 0 && g_rotated_count == 0) {
        return;
    }

    if (g_axis_count > 0) {
        void *data = SDL_MapGPUTransferBuffer(g_device, g_axis_transfer, true);
        memcpy(data, g_axis_instances, g_axis_count * sizeof(axisinstance_t));
        SDL_UnmapGPUTransferBuffer(g_device, g_axis_transfer);
    }

    if (g_rotated_count > 0) {
        void *data = SDL_MapGPUTransferBuffer(g_device, g_rotated_transfer, true);
        memcpy(data, g_rotated_instances, g_rotated_count * sizeof(sirotated_instance_t));
        SDL_UnmapGPUTransferBuffer(g_device, g_rotated_transfer);
    }

    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(g_command_buffer);

    if (g_axis_count > 0) {
        SDL_UploadToGPUBuffer(
            copy,
            &(SDL_GPUTransferBufferLocation){ .transfer_buffer = g_axis_transfer },
            &(SDL_GPUBufferRegion){ .buffer = g_axis_buffer,
                                    .size = g_axis_count * sizeof(axisinstance_t) },
            true
        );
    }

    if (g_rotated_count > 0) {
        SDL_UploadToGPUBuffer(
            copy,
            &(SDL_GPUTransferBufferLocation){ .transfer_buffer = g_rotated_transfer },
            &(SDL_GPUBufferRegion){
                .buffer = g_rotated_buffer,
                .size = g_rotated_count * sizeof(sirotated_instance_t),
            },
            true
        );
    }

    SDL_EndGPUCopyPass(copy);
}

static void bind_and_draw(
    SDL_GPURenderPass *pass,
    SDL_GPUGraphicsPipeline *pipeline,
    SDL_GPUBuffer *instance_buffer,
    Uint32 count
) {
    if (count == 0) {
        return;
    }

    SDL_GPUBufferBinding vertex_bindings[2] = {
        { .buffer = g_vertex_buffer },
        { .buffer = instance_buffer },
    };
    SDL_GPUBufferBinding index_binding = { .buffer = g_index_buffer };
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_BindGPUVertexBuffers(pass, 0, vertex_bindings, 2);
    SDL_BindGPUIndexBuffer(pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_DrawGPUIndexedPrimitives(pass, 36, count, 0, 0, 0);
}

static void draw_shadow_pass(void) {
    if (!g_shadows_enabled || (g_axis_count == 0 && g_rotated_count == 0)) {
        return;
    }

    SDL_GPUDepthStencilTargetInfo depth_target = {
        .texture = g_shadow_texture,
        .clear_depth = 1.0f,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
        .cycle = true,
    };
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(g_command_buffer, NULL, 0, &depth_target);
    SDL_PushGPUVertexUniformData(
        g_command_buffer,
        0,
        &g_light_view_projection,
        sizeof(g_light_view_projection)
    );
    bind_and_draw(pass, g_axis_shadow_pipeline, g_axis_buffer, g_axis_count);
    bind_and_draw(pass, g_rotated_shadow_pipeline, g_rotated_buffer, g_rotated_count);
    SDL_EndGPURenderPass(pass);
}

static void draw_main_pass(void) {
    bool msaa = g_sample_count != SDL_GPU_SAMPLECOUNT_1;
    SDL_GPUColorTargetInfo color_targets[2] = {
        {
            .texture = msaa ? g_msaa_texture : g_scene_texture,
            .clear_color = g_sky_linear,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = msaa ? SDL_GPU_STOREOP_RESOLVE : SDL_GPU_STOREOP_STORE,
            .resolve_texture = msaa ? g_scene_texture : NULL,
            .cycle = msaa,
        },
        {
            .texture = msaa ? g_bloom_msaa_texture : g_bloom_texture,
            .clear_color = { 0.0f, 0.0f, 0.0f, 0.0f },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = msaa ? SDL_GPU_STOREOP_RESOLVE : SDL_GPU_STOREOP_STORE,
            .resolve_texture = msaa ? g_bloom_texture : NULL,
            .cycle = msaa,
        },
    };
    SDL_GPUDepthStencilTargetInfo depth_target = {
        .texture = g_depth_texture,
        .clear_depth = 1.0f,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_DONT_CARE,
        .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
        .cycle = true,
    };
    SDL_GPURenderPass *pass =
        SDL_BeginGPURenderPass(g_command_buffer, color_targets, 2, &depth_target);
    transform_uniform_t transforms = {
        .view_projection = g_view_projection,
        .light_view_projection = g_light_view_projection,
    };
    lighting_uniform_t lighting = {
        .camera_position = { g_camera.position.x, g_camera.position.y, g_camera.position.z, 1.0f },
        .sun_direction_intensity = {
            g_sun_direction.x,
            g_sun_direction.y,
            g_sun_direction.z,
            g_sun_intensity,
        },
        .sun_color = { g_sun_color.r, g_sun_color.g, g_sun_color.b, 1.0f },
        .ambient_color_intensity = {
            g_ambient_color.r,
            g_ambient_color.g,
            g_ambient_color.b,
            g_ambient_intensity,
        },
        .fog_color = { g_fog_color.r, g_fog_color.g, g_fog_color.b, 1.0f },
        .fog_parameters = {
            g_fog_start,
            g_fog_end,
            g_fog_enabled ? 1.0f : 0.0f,
            g_shadows_enabled ? 1.0f : 0.0f,
        },
        .shadow_parameters = {
            g_shadow_distance,
            0.0f,
            0.0f,
            0.0f,
        },
    };
    SDL_GPUTextureSamplerBinding shadow_binding = {
        .texture = g_shadow_texture,
        .sampler = g_shadow_sampler,
    };
    SDL_PushGPUVertexUniformData(g_command_buffer, 0, &transforms, sizeof(transforms));
    SDL_PushGPUFragmentUniformData(g_command_buffer, 0, &lighting, sizeof(lighting));
    SDL_BindGPUFragmentSamplers(pass, 0, &shadow_binding, 1);
    bind_and_draw(pass, g_axis_pipeline, g_axis_buffer, g_axis_count);
    bind_and_draw(pass, g_rotated_pipeline, g_rotated_buffer, g_rotated_count);
    SDL_EndGPURenderPass(pass);
}

static void draw_fullscreen(
    SDL_GPUGraphicsPipeline *pipeline,
    SDL_GPUTexture *target,
    Uint32 width,
    Uint32 height,
    const SDL_GPUTextureSamplerBinding *samplers,
    Uint32 sampler_count,
    const float *uniform,
    Uint32 uniform_size
) {
    SDL_GPUColorTargetInfo color_target = {
        .texture = target,
        .load_op = SDL_GPU_LOADOP_DONT_CARE,
        .store_op = SDL_GPU_STOREOP_STORE,
        .cycle = true,
    };
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(g_command_buffer, &color_target, 1, NULL);
    SDL_SetGPUViewport(
        pass,
        &(SDL_GPUViewport){
            .w = (float)width,
            .h = (float)height,
            .min_depth = 0.0f,
            .max_depth = 1.0f,
        }
    );
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_BindGPUFragmentSamplers(pass, 0, samplers, sampler_count);
    SDL_PushGPUFragmentUniformData(g_command_buffer, 0, uniform, uniform_size);
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

static void bloom_downsample(
    SDL_GPUTexture *source,
    Uint32 source_width,
    Uint32 source_height,
    SDL_GPUTexture *target,
    Uint32 target_width,
    Uint32 target_height,
    float extract
) {
    SDL_GPUTextureSamplerBinding binding = {
        .texture = source,
        .sampler = g_bloom_sampler,
    };
    float uniform[4] = {
        g_bloom_threshold,
        extract,
        1.0f / (float)source_width,
        1.0f / (float)source_height,
    };
    draw_fullscreen(
        g_bloom_down_pipeline,
        target,
        target_width,
        target_height,
        &binding,
        1,
        uniform,
        sizeof(uniform)
    );
}

static void bloom_blur(
    SDL_GPUTexture *source,
    SDL_GPUTexture *scratch,
    SDL_GPUTexture *target,
    Uint32 width,
    Uint32 height
) {
    SDL_GPUTextureSamplerBinding binding = {
        .texture = source,
        .sampler = g_bloom_sampler,
    };
    float horizontal[4] = { 1.0f / (float)width, 0.0f, 0.0f, 0.0f };
    draw_fullscreen(
        g_bloom_blur_pipeline,
        scratch,
        width,
        height,
        &binding,
        1,
        horizontal,
        sizeof(horizontal)
    );
    binding.texture = scratch;
    float vertical[4] = { 0.0f, 1.0f / (float)height, 0.0f, 0.0f };
    draw_fullscreen(
        g_bloom_blur_pipeline,
        target,
        width,
        height,
        &binding,
        1,
        vertical,
        sizeof(vertical)
    );
}

static void draw_bloom_passes(void) {
    if (!g_bloom_enabled || !g_any_bloom) {
        return;
    }

    bloom_downsample(
        g_bloom_texture,
        g_frame_width,
        g_frame_height,
        g_bloom_half_scratch,
        g_bloom_half_width,
        g_bloom_half_height,
        1.0f
    );
    bloom_blur(
        g_bloom_half_scratch,
        g_bloom_half,
        g_bloom_half_scratch,
        g_bloom_half_width,
        g_bloom_half_height
    );
    bloom_downsample(
        g_bloom_half_scratch,
        g_bloom_half_width,
        g_bloom_half_height,
        g_bloom_quarter_scratch,
        g_bloom_quarter_width,
        g_bloom_quarter_height,
        0.0f
    );
    bloom_blur(
        g_bloom_quarter_scratch,
        g_bloom_quarter,
        g_bloom_quarter_scratch,
        g_bloom_quarter_width,
        g_bloom_quarter_height
    );
}

static void draw_composite_pass(void) {
    SDL_GPUTexture *half = g_any_bloom && g_bloom_enabled ? g_bloom_half_scratch : g_bloom_texture;
    SDL_GPUTexture *quarter = g_any_bloom && g_bloom_enabled ? g_bloom_quarter_scratch : g_bloom_texture;
    SDL_GPUTextureSamplerBinding samplers[3] = {
        { .texture = g_scene_texture, .sampler = g_bloom_sampler },
        { .texture = half, .sampler = g_bloom_sampler },
        { .texture = quarter, .sampler = g_bloom_sampler },
    };
    float uniform[4] = {
        g_bloom_enabled && g_any_bloom ? g_bloom_intensity : 0.0f,
        g_linear_swapchain ? 0.0f : 1.0f,
        0.0f,
        0.0f,
    };
    SDL_GPUColorTargetInfo color_target = {
        .texture = g_swapchain,
        .load_op = SDL_GPU_LOADOP_DONT_CARE,
        .store_op = SDL_GPU_STOREOP_STORE,
    };
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(g_command_buffer, &color_target, 1, NULL);
    SDL_BindGPUGraphicsPipeline(pass, g_bloom_composite_pipeline);
    SDL_BindGPUFragmentSamplers(pass, 0, samplers, 3);
    SDL_PushGPUFragmentUniformData(g_command_buffer, 0, uniform, sizeof(uniform));
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

void sigpu_init(const char *title, int width, int height) {
    for (int index = 0; index < 256; index++) {
        float srgb = index / 255.0f;
        float linear = srgb <= 0.04045f ? srgb / 12.92f : powf((srgb + 0.055f) / 1.055f, 2.4f);
        g_linear_lut[index] = (uint8_t)roundf(linear * 255.0f);
    }

    SDL_Init(SDL_INIT_VIDEO);
    g_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
    g_window = SDL_CreateWindow(
        title,
        width,
        height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );
    SDL_ClaimWindowForGPUDevice(g_device, g_window);
    g_window_claimed = true;
    g_linear_swapchain = SDL_WindowSupportsGPUSwapchainComposition(
        g_device,
        g_window,
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR
    );
    SDL_SetGPUSwapchainParameters(
        g_device,
        g_window,
        g_linear_swapchain ? SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR
                           : SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
        SDL_GPU_PRESENTMODE_VSYNC
    );
    g_swapchain_format = SDL_GetGPUSwapchainTextureFormat(g_device, g_window);
    g_sample_count = SDL_GPU_SAMPLECOUNT_1;
    create_cube_mesh();
    resize_axis_instances(SIGPU_AXIS_CAPACITY);
    resize_rotated_instances(SIGPU_ROTATED_CAPACITY);
    create_main_pipelines();
    create_shadow_resources();
    create_bloom_resources();
    sigpu_camera(0.0f, 2.0f, -6.0f, 0.0f, 0.0f, 0.0f, 60.0f);
    sigpu_sky(sigpu_rgb(13, 13, 20));
    sigpu_sun(-1.0f, -2.0f, 1.0f, sigpu_rgb(255, 245, 220), 1.0f);
    sigpu_ambient(sigpu_rgb(255, 255, 255), 0.2f);
    g_shadow_distance = 35.0f;
    g_fog_color = linear_color(sigpu_rgb(13, 13, 20));
    g_fog_start = 0.0f;
    g_fog_end = 0.0f;
    g_fog_enabled = false;
    g_shadows_enabled = false;
    g_bloom_enabled = true;
    g_bloom_threshold = 0.0f;
    g_bloom_intensity = 1.0f;
    g_light_view_projection = mat4_identity();
}

void sigpu_camera(
    float x,
    float y,
    float z,
    float target_x,
    float target_y,
    float target_z,
    float fov
) {
    g_camera.position = (vec3_t){ x, y, z };
    g_camera.target = (vec3_t){ target_x, target_y, target_z };
    g_camera.fov = fminf(fmaxf(fov, 1.0f), 179.0f);
    g_camera.near_plane = 0.1f;
    g_camera.far_plane = 1000.0f;
}

void sigpu_sky(sigpu_color_t color) {
    g_sky_linear = linear_color(color);
    g_sky_srgb =
        (SDL_FColor){ color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
}

void sigpu_sun(float dx, float dy, float dz, sigpu_color_t color, float intensity) {
    g_sun_direction = vec3_normalize((vec3_t){ dx, dy, dz });
    g_sun_color = linear_color(color);
    g_sun_intensity = intensity;
}

void sigpu_ambient(sigpu_color_t color, float intensity) {
    g_ambient_color = linear_color(color);
    g_ambient_intensity = intensity;
}

void sigpu_fog(sigpu_color_t color, float start, float end) {
    g_fog_color = linear_color(color);
    g_fog_start = start;
    g_fog_end = end;
    g_fog_enabled = end > start;
}

void sigpu_shadows(bool enabled) { g_shadows_enabled = enabled; }

void sigpu_shadow_distance(float distance) { g_shadow_distance = distance; }

void sigpu_bloom(bool enabled, float threshold, float intensity) {
    g_bloom_enabled = enabled;
    g_bloom_threshold = threshold;
    g_bloom_intensity = intensity;
}

static SDL_GPUSampleCount sample_count_from_int(int samples) {
    switch (samples) {
    case 8:
        return SDL_GPU_SAMPLECOUNT_8;
    case 4:
        return SDL_GPU_SAMPLECOUNT_4;
    case 2:
        return SDL_GPU_SAMPLECOUNT_2;
    default:
        return SDL_GPU_SAMPLECOUNT_1;
    }
}

static SDL_GPUSampleCount lower_sample_count(SDL_GPUSampleCount samples) {
    switch (samples) {
    case SDL_GPU_SAMPLECOUNT_8:
        return SDL_GPU_SAMPLECOUNT_4;
    case SDL_GPU_SAMPLECOUNT_4:
        return SDL_GPU_SAMPLECOUNT_2;
    default:
        return SDL_GPU_SAMPLECOUNT_1;
    }
}

void sigpu_msaa(int samples) {
    SDL_GPUSampleCount selected = sample_count_from_int(samples);

    while (
        selected != SDL_GPU_SAMPLECOUNT_1 &&
        (!SDL_GPUTextureSupportsSampleCount(g_device, SIGPU_HDR_FORMAT, selected) ||
         !SDL_GPUTextureSupportsSampleCount(g_device, SDL_GPU_TEXTUREFORMAT_D16_UNORM, selected))) {
        selected = lower_sample_count(selected);
    }

    if (selected != g_sample_count) {
        g_sample_count = selected;
        release_frame_targets();
        create_main_pipelines();
    }
}

bool sigpu_begin_frame(void) {
    SDL_Event event;
    bool running = true;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            running = false;
        }

        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
            running = false;
        }
    }

    g_axis_count = 0;
    g_rotated_count = 0;
    g_any_bloom = false;
    g_command_buffer = SDL_AcquireGPUCommandBuffer(g_device);
    g_swapchain = NULL;
    SDL_WaitAndAcquireGPUSwapchainTexture(
        g_command_buffer,
        g_window,
        &g_swapchain,
        &g_frame_width,
        &g_frame_height
    );
    return running;
}

void sigpu_cube(
    float x,
    float y,
    float z,
    float width,
    float height,
    float depth,
    sigpu_color_t color,
    float bloom
) {
    if (g_axis_count == g_axis_capacity) {
        resize_axis_instances(g_axis_capacity * 2);
    }

    axisinstance_t *cube = &g_axis_instances[g_axis_count++];
    cube->x = x;
    cube->y = y;
    cube->z = z;
    cube->width = width;
    cube->height = height;
    cube->depth = depth;
    pack_color(&cube->r, color);
    cube->bloom = fmaxf(bloom, 0.0f);
    g_any_bloom = g_any_bloom || cube->bloom > 0.0f;
}

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
    sigpu_color_t color,
    float bloom
) {
    if (g_rotated_count == g_rotated_capacity) {
        resize_rotated_instances(g_rotated_capacity * 2);
    }

    float half_x = rx * SIGPU_PI / 360.0f;
    float half_y = ry * SIGPU_PI / 360.0f;
    float half_z = rz * SIGPU_PI / 360.0f;
    float sx = sinf(half_x);
    float cx = cosf(half_x);
    float sy = sinf(half_y);
    float cy = cosf(half_y);
    float sz = sinf(half_z);
    float cz = cosf(half_z);
    sirotated_instance_t *cube = &g_rotated_instances[g_rotated_count++];
    cube->x = x;
    cube->y = y;
    cube->z = z;
    cube->width = width;
    cube->height = height;
    cube->depth = depth;
    cube->qx = (int16_t)roundf((sx * cy * cz - cx * sy * sz) * 32767.0f);
    cube->qy = (int16_t)roundf((cx * sy * cz + sx * cy * sz) * 32767.0f);
    cube->qz = (int16_t)roundf((cx * cy * sz - sx * sy * cz) * 32767.0f);
    cube->qw = (int16_t)roundf((cx * cy * cz + sx * sy * sz) * 32767.0f);
    pack_color(&cube->r, color);
    cube->bloom = fmaxf(bloom, 0.0f);
    g_any_bloom = g_any_bloom || cube->bloom > 0.0f;
}

void sigpu_end_frame(void) {
    if (!g_swapchain) {
        SDL_SubmitGPUCommandBuffer(g_command_buffer);
        g_command_buffer = NULL;
        return;
    }

    ensure_frame_targets();
    float aspect = (float)g_frame_width / (float)g_frame_height;
    g_view = mat4_look_at_lh(g_camera.position, g_camera.target, (vec3_t){ 0.0f, 1.0f, 0.0f });
    mat4_t projection =
        mat4_perspective_lh(g_camera.fov, aspect, g_camera.near_plane, g_camera.far_plane);
    g_view_projection = mat4_mul(projection, g_view);

    if (g_shadows_enabled) {
        build_shadow_transform(aspect);
    } else {
        g_light_view_projection = mat4_identity();
    }

    compact_instances(aspect);
    upload_instances();
    draw_shadow_pass();
    draw_main_pass();
    draw_bloom_passes();
    draw_composite_pass();
    SDL_SubmitGPUCommandBuffer(g_command_buffer);
    g_command_buffer = NULL;
    g_swapchain = NULL;
}

void sigpu_fini(void) {
    SDL_WaitForGPUIdle(g_device);
    SDL_ReleaseGPUGraphicsPipeline(g_device, g_axis_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(g_device, g_rotated_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(g_device, g_axis_shadow_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(g_device, g_rotated_shadow_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(g_device, g_bloom_down_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(g_device, g_bloom_blur_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(g_device, g_bloom_composite_pipeline);
    SDL_ReleaseGPUBuffer(g_device, g_vertex_buffer);
    SDL_ReleaseGPUBuffer(g_device, g_index_buffer);
    SDL_ReleaseGPUBuffer(g_device, g_axis_buffer);
    SDL_ReleaseGPUBuffer(g_device, g_rotated_buffer);
    SDL_ReleaseGPUTransferBuffer(g_device, g_axis_transfer);
    SDL_ReleaseGPUTransferBuffer(g_device, g_rotated_transfer);
    SDL_ReleaseGPUTexture(g_device, g_shadow_texture);
    SDL_ReleaseGPUSampler(g_device, g_shadow_sampler);
    SDL_ReleaseGPUSampler(g_device, g_bloom_sampler);
    release_frame_targets();

    if (g_window_claimed) {
        SDL_ReleaseWindowFromGPUDevice(g_device, g_window);
    }

    free(g_axis_instances);
    free(g_rotated_instances);
    SDL_DestroyWindow(g_window);
    SDL_DestroyGPUDevice(g_device);
    SDL_Quit();
}
