#include "sigpu_internal.h"

#include <string.h>

static void upload_instances(void) {
    if (g_sigpu.axis_count == 0 && g_sigpu.rotated_count == 0) {
        return;
    }

    if (g_sigpu.axis_count > 0) {
        void *data = SDL_MapGPUTransferBuffer(g_sigpu.device, g_sigpu.axis_transfer, true);
        memcpy(data, g_sigpu.axis_instances, g_sigpu.axis_count * sizeof(sigpu_axis_instance_t));
        SDL_UnmapGPUTransferBuffer(g_sigpu.device, g_sigpu.axis_transfer);
    }

    if (g_sigpu.rotated_count > 0) {
        void *data = SDL_MapGPUTransferBuffer(g_sigpu.device, g_sigpu.rotated_transfer, true);
        memcpy(
            data,
            g_sigpu.rotated_instances,
            g_sigpu.rotated_count * sizeof(sigpu_rotated_instance_t)
        );
        SDL_UnmapGPUTransferBuffer(g_sigpu.device, g_sigpu.rotated_transfer);
    }

    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(g_sigpu.command_buffer);

    if (g_sigpu.axis_count > 0) {
        SDL_UploadToGPUBuffer(
            copy,
            &(SDL_GPUTransferBufferLocation){ .transfer_buffer = g_sigpu.axis_transfer },
            &(SDL_GPUBufferRegion){ .buffer = g_sigpu.axis_buffer,
                                    .size = g_sigpu.axis_count * sizeof(sigpu_axis_instance_t) },
            true
        );
    }

    if (g_sigpu.rotated_count > 0) {
        SDL_UploadToGPUBuffer(
            copy,
            &(SDL_GPUTransferBufferLocation){ .transfer_buffer = g_sigpu.rotated_transfer },
            &(SDL_GPUBufferRegion){
                .buffer = g_sigpu.rotated_buffer,
                .size = g_sigpu.rotated_count * sizeof(sigpu_rotated_instance_t),
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
        { .buffer = g_sigpu.vertex_buffer },
        { .buffer = instance_buffer },
    };
    SDL_GPUBufferBinding index_binding = { .buffer = g_sigpu.index_buffer };
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_BindGPUVertexBuffers(pass, 0, vertex_bindings, 2);
    SDL_BindGPUIndexBuffer(pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_DrawGPUIndexedPrimitives(pass, 36, count, 0, 0, 0);
}

static void draw_shadow_pass(void) {
    if (!g_sigpu.shadows_enabled || (g_sigpu.axis_count == 0 && g_sigpu.rotated_count == 0)) {
        return;
    }

    SDL_GPUDepthStencilTargetInfo depth_target = {
        .texture = g_sigpu.shadow_texture,
        .clear_depth = 1.0f,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
        .cycle = true,
    };
    SDL_GPURenderPass *pass =
        SDL_BeginGPURenderPass(g_sigpu.command_buffer, NULL, 0, &depth_target);
    SDL_PushGPUVertexUniformData(
        g_sigpu.command_buffer,
        0,
        &g_sigpu.light_view_projection,
        sizeof(g_sigpu.light_view_projection)
    );
    bind_and_draw(pass, g_sigpu.axis_shadow_pipeline, g_sigpu.axis_buffer, g_sigpu.axis_count);
    bind_and_draw(
        pass,
        g_sigpu.rotated_shadow_pipeline,
        g_sigpu.rotated_buffer,
        g_sigpu.rotated_count
    );
    SDL_EndGPURenderPass(pass);
}

static void draw_main_pass(void) {
    bool msaa = g_sigpu.sample_count != SDL_GPU_SAMPLECOUNT_1;
    SDL_GPUColorTargetInfo color_targets[2] = {
        {
            .texture = msaa ? g_sigpu.msaa_texture : g_sigpu.scene_texture,
            .clear_color = g_sigpu.sky_linear,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = msaa ? SDL_GPU_STOREOP_RESOLVE : SDL_GPU_STOREOP_STORE,
            .resolve_texture = msaa ? g_sigpu.scene_texture : NULL,
            .cycle = msaa,
        },
        {
            .texture = msaa ? g_sigpu.bloom_msaa_texture : g_sigpu.bloom_texture,
            .clear_color = { 0.0f, 0.0f, 0.0f, 0.0f },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = msaa ? SDL_GPU_STOREOP_RESOLVE : SDL_GPU_STOREOP_STORE,
            .resolve_texture = msaa ? g_sigpu.bloom_texture : NULL,
            .cycle = msaa,
        },
    };
    SDL_GPUDepthStencilTargetInfo depth_target = {
        .texture = g_sigpu.depth_texture,
        .clear_depth = 1.0f,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_DONT_CARE,
        .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
        .cycle = true,
    };
    SDL_GPURenderPass *pass =
        SDL_BeginGPURenderPass(g_sigpu.command_buffer, color_targets, 2, &depth_target);
    sigpu_transform_uniform_t transforms = {
        .view_projection = g_sigpu.view_projection,
        .light_view_projection = g_sigpu.light_view_projection,
    };
    sigpu_lighting_uniform_t lighting = {
        .camera_position = { g_sigpu.camera.position.x, g_sigpu.camera.position.y, g_sigpu.camera.position.z, 1.0f },
        .sun_direction_intensity = {
            g_sigpu.sun_direction.x,
            g_sigpu.sun_direction.y,
            g_sigpu.sun_direction.z,
            g_sigpu.sun_intensity,
        },
        .sun_color = { g_sigpu.sun_color.r, g_sigpu.sun_color.g, g_sigpu.sun_color.b, 1.0f },
        .ambient_color_intensity = {
            g_sigpu.ambient_color.r,
            g_sigpu.ambient_color.g,
            g_sigpu.ambient_color.b,
            g_sigpu.ambient_intensity,
        },
        .fog_color = { g_sigpu.fog_color.r, g_sigpu.fog_color.g, g_sigpu.fog_color.b, 1.0f },
        .fog_parameters = {
            g_sigpu.fog_start,
            g_sigpu.fog_end,
            g_sigpu.fog_enabled ? 1.0f : 0.0f,
            g_sigpu.shadows_enabled ? 1.0f : 0.0f,
        },
        .shadow_parameters = {
            g_sigpu.shadow_distance,
            0.0f,
            0.0f,
            0.0f,
        },
    };
    SDL_GPUTextureSamplerBinding shadow_binding = {
        .texture = g_sigpu.shadow_texture,
        .sampler = g_sigpu.shadow_sampler,
    };
    SDL_PushGPUVertexUniformData(g_sigpu.command_buffer, 0, &transforms, sizeof(transforms));
    SDL_PushGPUFragmentUniformData(g_sigpu.command_buffer, 0, &lighting, sizeof(lighting));
    SDL_BindGPUFragmentSamplers(pass, 0, &shadow_binding, 1);
    bind_and_draw(pass, g_sigpu.axis_pipeline, g_sigpu.axis_buffer, g_sigpu.axis_count);
    bind_and_draw(pass, g_sigpu.rotated_pipeline, g_sigpu.rotated_buffer, g_sigpu.rotated_count);
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
    SDL_GPURenderPass *pass =
        SDL_BeginGPURenderPass(g_sigpu.command_buffer, &color_target, 1, NULL);
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
    SDL_PushGPUFragmentUniformData(g_sigpu.command_buffer, 0, uniform, uniform_size);
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
        .sampler = g_sigpu.bloom_sampler,
    };
    float uniform[4] = {
        g_sigpu.bloom_threshold,
        extract,
        1.0f / (float)source_width,
        1.0f / (float)source_height,
    };
    draw_fullscreen(
        g_sigpu.bloom_down_pipeline,
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
        .sampler = g_sigpu.bloom_sampler,
    };
    float horizontal[4] = { 1.0f / (float)width, 0.0f, 0.0f, 0.0f };
    draw_fullscreen(
        g_sigpu.bloom_blur_pipeline,
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
        g_sigpu.bloom_blur_pipeline,
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
    if (!g_sigpu.bloom_enabled || !g_sigpu.any_bloom) {
        return;
    }

    bloom_downsample(
        g_sigpu.bloom_texture,
        g_sigpu.frame_width,
        g_sigpu.frame_height,
        g_sigpu.bloom_half_scratch,
        g_sigpu.bloom_half_width,
        g_sigpu.bloom_half_height,
        1.0f
    );
    bloom_blur(
        g_sigpu.bloom_half_scratch,
        g_sigpu.bloom_half,
        g_sigpu.bloom_half_scratch,
        g_sigpu.bloom_half_width,
        g_sigpu.bloom_half_height
    );
    bloom_downsample(
        g_sigpu.bloom_half_scratch,
        g_sigpu.bloom_half_width,
        g_sigpu.bloom_half_height,
        g_sigpu.bloom_quarter_scratch,
        g_sigpu.bloom_quarter_width,
        g_sigpu.bloom_quarter_height,
        0.0f
    );
    bloom_blur(
        g_sigpu.bloom_quarter_scratch,
        g_sigpu.bloom_quarter,
        g_sigpu.bloom_quarter_scratch,
        g_sigpu.bloom_quarter_width,
        g_sigpu.bloom_quarter_height
    );
}

static void draw_composite_pass(void) {
    SDL_GPUTexture *half = g_sigpu.any_bloom && g_sigpu.bloom_enabled ? g_sigpu.bloom_half_scratch
                                                                      : g_sigpu.bloom_texture;
    SDL_GPUTexture *quarter = g_sigpu.any_bloom && g_sigpu.bloom_enabled
                                  ? g_sigpu.bloom_quarter_scratch
                                  : g_sigpu.bloom_texture;
    SDL_GPUTextureSamplerBinding samplers[3] = {
        { .texture = g_sigpu.scene_texture, .sampler = g_sigpu.bloom_sampler },
        { .texture = half, .sampler = g_sigpu.bloom_sampler },
        { .texture = quarter, .sampler = g_sigpu.bloom_sampler },
    };
    float uniform[4] = {
        g_sigpu.bloom_enabled && g_sigpu.any_bloom ? g_sigpu.bloom_intensity : 0.0f,
        g_sigpu.linear_swapchain ? 0.0f : 1.0f,
        0.0f,
        0.0f,
    };
    SDL_GPUColorTargetInfo color_target = {
        .texture = g_sigpu.swapchain,
        .load_op = SDL_GPU_LOADOP_DONT_CARE,
        .store_op = SDL_GPU_STOREOP_STORE,
    };
    SDL_GPURenderPass *pass =
        SDL_BeginGPURenderPass(g_sigpu.command_buffer, &color_target, 1, NULL);
    SDL_BindGPUGraphicsPipeline(pass, g_sigpu.bloom_composite_pipeline);
    SDL_BindGPUFragmentSamplers(pass, 0, samplers, 3);
    SDL_PushGPUFragmentUniformData(g_sigpu.command_buffer, 0, uniform, sizeof(uniform));
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

void sigpu_passes_draw(void) {
    upload_instances();
    draw_shadow_pass();
    draw_main_pass();
    draw_bloom_passes();
    draw_composite_pass();
}
