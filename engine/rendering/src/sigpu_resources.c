#include "sigpu_internal.h"

#include <stdlib.h>
#include <string.h>

static const sigpu_cube_vertex_t cube_vertices[24] = {
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

static void create_cube_mesh(void) {
    Uint32 vertex_size = sizeof(cube_vertices);
    Uint32 index_size = sizeof(cube_indices);
    SDL_GPUTransferBuffer *transfer = SDL_CreateGPUTransferBuffer(
        g_sigpu.device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = vertex_size + index_size,
        }
    );
    g_sigpu.vertex_buffer = SDL_CreateGPUBuffer(
        g_sigpu.device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = vertex_size,
        }
    );
    g_sigpu.index_buffer = SDL_CreateGPUBuffer(
        g_sigpu.device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = index_size,
        }
    );
    uint8_t *data = SDL_MapGPUTransferBuffer(g_sigpu.device, transfer, false);
    memcpy(data, cube_vertices, vertex_size);
    memcpy(data + vertex_size, cube_indices, index_size);
    SDL_UnmapGPUTransferBuffer(g_sigpu.device, transfer);
    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(g_sigpu.device);
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(command_buffer);
    SDL_UploadToGPUBuffer(
        copy,
        &(SDL_GPUTransferBufferLocation){ .transfer_buffer = transfer },
        &(SDL_GPUBufferRegion){ .buffer = g_sigpu.vertex_buffer, .size = vertex_size },
        false
    );
    SDL_UploadToGPUBuffer(
        copy,
        &(SDL_GPUTransferBufferLocation){ .transfer_buffer = transfer, .offset = vertex_size },
        &(SDL_GPUBufferRegion){ .buffer = g_sigpu.index_buffer, .size = index_size },
        false
    );
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    SDL_ReleaseGPUTransferBuffer(g_sigpu.device, transfer);
}

static void *resize_instances(
    void *instances,
    SDL_GPUBuffer **buffer,
    SDL_GPUTransferBuffer **transfer,
    Uint32 capacity,
    Uint32 stride
) {
    instances = realloc(instances, stride * capacity);

    if (*buffer) {
        SDL_ReleaseGPUBuffer(g_sigpu.device, *buffer);
        SDL_ReleaseGPUTransferBuffer(g_sigpu.device, *transfer);
    }

    Uint32 size = stride * capacity;
    *buffer = SDL_CreateGPUBuffer(
        g_sigpu.device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = size,
        }
    );
    *transfer = SDL_CreateGPUTransferBuffer(
        g_sigpu.device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = size,
        }
    );
    return instances;
}

static void resize_axis_instances(Uint32 capacity) {
    g_sigpu.axis_instances = resize_instances(
        g_sigpu.axis_instances,
        &g_sigpu.axis_buffer,
        &g_sigpu.axis_transfer,
        capacity,
        sizeof(*g_sigpu.axis_instances)
    );
    g_sigpu.axis_capacity = capacity;
}

static void resize_rotated_instances(Uint32 capacity) {
    g_sigpu.rotated_instances = resize_instances(
        g_sigpu.rotated_instances,
        &g_sigpu.rotated_buffer,
        &g_sigpu.rotated_transfer,
        capacity,
        sizeof(*g_sigpu.rotated_instances)
    );
    g_sigpu.rotated_capacity = capacity;
}

static SDL_GPUTexture *create_hdr_texture(
    Uint32 width,
    Uint32 height,
    SDL_GPUTextureUsageFlags usage,
    SDL_GPUSampleCount sample_count
) {
    return SDL_CreateGPUTexture(
        g_sigpu.device,
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
        SDL_ReleaseGPUTexture(g_sigpu.device, *texture);
        *texture = NULL;
    }
}

static void release_frame_targets(void) {
    release_texture(&g_sigpu.depth_texture);
    release_texture(&g_sigpu.msaa_texture);
    release_texture(&g_sigpu.bloom_msaa_texture);
    release_texture(&g_sigpu.scene_texture);
    release_texture(&g_sigpu.bloom_texture);
    release_texture(&g_sigpu.bloom_half);
    release_texture(&g_sigpu.bloom_half_scratch);
    release_texture(&g_sigpu.bloom_quarter);
    release_texture(&g_sigpu.bloom_quarter_scratch);

    g_sigpu.target_width = 0;
    g_sigpu.target_height = 0;
}

static void ensure_frame_targets(void) {
    if (g_sigpu.scene_texture && g_sigpu.target_width == g_sigpu.frame_width &&
        g_sigpu.target_height == g_sigpu.frame_height) {
        return;
    }

    release_frame_targets();
    g_sigpu.depth_texture = SDL_CreateGPUTexture(
        g_sigpu.device,
        &(SDL_GPUTextureCreateInfo){
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
            .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
            .width = g_sigpu.frame_width,
            .height = g_sigpu.frame_height,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = g_sigpu.sample_count,
        }
    );
    g_sigpu.scene_texture = create_hdr_texture(
        g_sigpu.frame_width,
        g_sigpu.frame_height,
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        SDL_GPU_SAMPLECOUNT_1
    );

    if (g_sigpu.sample_count != SDL_GPU_SAMPLECOUNT_1) {
        g_sigpu.msaa_texture = create_hdr_texture(
            g_sigpu.frame_width,
            g_sigpu.frame_height,
            SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
            g_sigpu.sample_count
        );
        g_sigpu.bloom_msaa_texture = create_hdr_texture(
            g_sigpu.frame_width,
            g_sigpu.frame_height,
            SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
            g_sigpu.sample_count
        );
    }

    g_sigpu.bloom_texture = create_hdr_texture(
        g_sigpu.frame_width,
        g_sigpu.frame_height,
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        SDL_GPU_SAMPLECOUNT_1
    );
    g_sigpu.bloom_half_width = g_sigpu.frame_width > 1 ? g_sigpu.frame_width / 2 : 1;
    g_sigpu.bloom_half_height = g_sigpu.frame_height > 1 ? g_sigpu.frame_height / 2 : 1;
    g_sigpu.bloom_quarter_width = g_sigpu.bloom_half_width > 1 ? g_sigpu.bloom_half_width / 2 : 1;
    g_sigpu.bloom_quarter_height =
        g_sigpu.bloom_half_height > 1 ? g_sigpu.bloom_half_height / 2 : 1;
    g_sigpu.bloom_half = create_hdr_texture(
        g_sigpu.bloom_half_width,
        g_sigpu.bloom_half_height,
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        SDL_GPU_SAMPLECOUNT_1
    );
    g_sigpu.bloom_half_scratch = create_hdr_texture(
        g_sigpu.bloom_half_width,
        g_sigpu.bloom_half_height,
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        SDL_GPU_SAMPLECOUNT_1
    );
    g_sigpu.bloom_quarter = create_hdr_texture(
        g_sigpu.bloom_quarter_width,
        g_sigpu.bloom_quarter_height,
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        SDL_GPU_SAMPLECOUNT_1
    );
    g_sigpu.bloom_quarter_scratch = create_hdr_texture(
        g_sigpu.bloom_quarter_width,
        g_sigpu.bloom_quarter_height,
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        SDL_GPU_SAMPLECOUNT_1
    );

    g_sigpu.target_width = g_sigpu.frame_width;
    g_sigpu.target_height = g_sigpu.frame_height;
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

static SDL_GPUSampleCount supported_sample_count(int samples) {
    SDL_GPUSampleCount selected = sample_count_from_int(samples);

    while (selected != SDL_GPU_SAMPLECOUNT_1 &&
           (!SDL_GPUTextureSupportsSampleCount(g_sigpu.device, SIGPU_HDR_FORMAT, selected) ||
            !SDL_GPUTextureSupportsSampleCount(
                g_sigpu.device,
                SDL_GPU_TEXTUREFORMAT_D16_UNORM,
                selected
            ))) {
        switch (selected) {
        case SDL_GPU_SAMPLECOUNT_8:
            selected = SDL_GPU_SAMPLECOUNT_4;
            break;
        case SDL_GPU_SAMPLECOUNT_4:
            selected = SDL_GPU_SAMPLECOUNT_2;
            break;
        default:
            selected = SDL_GPU_SAMPLECOUNT_1;
        }
    }
    return selected;
}

void sigpu_resources_create(int samples) {
    g_sigpu.sample_count = supported_sample_count(samples);
    create_cube_mesh();
    resize_axis_instances(SIGPU_AXIS_CAPACITY);
    resize_rotated_instances(SIGPU_ROTATED_CAPACITY);
    sigpu_pipelines_create();
}

void sigpu_resources_destroy(void) {
    sigpu_pipelines_destroy();
    SDL_ReleaseGPUBuffer(g_sigpu.device, g_sigpu.vertex_buffer);
    SDL_ReleaseGPUBuffer(g_sigpu.device, g_sigpu.index_buffer);
    SDL_ReleaseGPUBuffer(g_sigpu.device, g_sigpu.axis_buffer);
    SDL_ReleaseGPUBuffer(g_sigpu.device, g_sigpu.rotated_buffer);
    SDL_ReleaseGPUTransferBuffer(g_sigpu.device, g_sigpu.axis_transfer);
    SDL_ReleaseGPUTransferBuffer(g_sigpu.device, g_sigpu.rotated_transfer);
    release_frame_targets();
    free(g_sigpu.axis_instances);
    free(g_sigpu.rotated_instances);
}

void sigpu_axis_instances_grow(void) { resize_axis_instances(g_sigpu.axis_capacity * 2); }

void sigpu_rotated_instances_grow(void) { resize_rotated_instances(g_sigpu.rotated_capacity * 2); }

void sigpu_frame_targets_prepare(void) { ensure_frame_targets(); }

void sigpu_sample_count_set(int samples) {
    SDL_GPUSampleCount selected = supported_sample_count(samples);

    if (selected != g_sigpu.sample_count) {
        g_sigpu.sample_count = selected;
        release_frame_targets();
        sigpu_main_pipelines_recreate();
    }
}
