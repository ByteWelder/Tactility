// SPDX-License-Identifier: Apache-2.0
#include "sdl_display.h"

#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/drivers/display.h>
#include <tactility/log.h>
#include <tactility/module.h>

#include <SDL2/SDL.h>

#include <cstdlib>

constexpr auto* TAG = "SdlDisplay";
#define GET_CONFIG(device) (static_cast<const SdlDisplayConfig*>((device)->config))

struct SdlDisplayInternal {
    bool initialized;
    bool init_failed;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
};

// Only one sdl-display device exists in the simulator; sdl_input.cpp uses this to map canvas
// coordinates back to the fixed logical resolution SDL_RenderSetLogicalSize() scales to it.
static SdlDisplayInternal* g_display_internal = nullptr;

SDL_Renderer* sdl_display_get_renderer(void) {
    return g_display_internal != nullptr ? g_display_internal->renderer : nullptr;
}

void sdl_display_present_now(void) {
    if (g_display_internal == nullptr) {
        return;
    }
    SDL_RenderClear(g_display_internal->renderer);
    SDL_RenderCopy(g_display_internal->renderer, g_display_internal->texture, nullptr, nullptr);
    SDL_RenderPresent(g_display_internal->renderer);
}

// region Driver lifecycle

static error_t start(Device* device) {
    auto* internal = static_cast<SdlDisplayInternal*>(malloc(sizeof(SdlDisplayInternal)));
    if (internal == nullptr) {
        return ERROR_OUT_OF_MEMORY;
    }

    internal->initialized = false;
    internal->init_failed = false;
    internal->window = nullptr;
    internal->renderer = nullptr;
    internal->texture = nullptr;

    device_set_driver_data(device, internal);
    return ERROR_NONE;
}

static error_t stop(Device* device) {
    auto* internal = static_cast<SdlDisplayInternal*>(device_get_driver_data(device));

    if (internal->initialized) {
        SDL_DestroyTexture(internal->texture);
        SDL_DestroyRenderer(internal->renderer);
        SDL_DestroyWindow(internal->window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    if (g_display_internal == internal) {
        g_display_internal = nullptr;
    }

    free(internal);
    device_set_driver_data(device, nullptr);
    return ERROR_NONE;
}

// endregion

// region DisplayApi

static error_t sdl_display_reset(Device*) { return ERROR_NONE; }
static error_t sdl_display_init(Device*) { return ERROR_NONE; }

// Unlike the desktop simulator, all SDL calls happen on the LVGL task's own thread: SDL's
// Emscripten video driver proxies canvas operations to the browser main thread itself, so no
// application-level main-thread bridge is needed. The renderer is always software (the
// Emscripten video driver has no GPU renderer), which is also why SDL_RENDERER_SOFTWARE is
// passed explicitly.
static bool sdl_display_lazy_init(Device* device, SdlDisplayInternal* internal) {
    const auto* config = GET_CONFIG(device);

    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        LOG_E(TAG, "SDL_InitSubSystem failed: %s", SDL_GetError());
        return false;
    }

    internal->window = SDL_CreateWindow(
        "Tactility",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        config->horizontal_resolution, config->vertical_resolution,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    internal->renderer = internal->window != nullptr
        ? SDL_CreateRenderer(internal->window, -1, SDL_RENDERER_SOFTWARE)
        : nullptr;
    if (internal->renderer != nullptr) {
        SDL_RenderSetLogicalSize(internal->renderer, config->horizontal_resolution, config->vertical_resolution);
    }
    internal->texture = internal->renderer != nullptr
        ? SDL_CreateTexture(internal->renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
            config->horizontal_resolution, config->vertical_resolution)
        : nullptr;

    if (internal->window == nullptr || internal->renderer == nullptr || internal->texture == nullptr) {
        LOG_E(TAG, "Failed to create SDL window: %s", SDL_GetError());
        if (internal->texture != nullptr) SDL_DestroyTexture(internal->texture);
        if (internal->renderer != nullptr) SDL_DestroyRenderer(internal->renderer);
        if (internal->window != nullptr) SDL_DestroyWindow(internal->window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        return false;
    }

    g_display_internal = internal;
    return true;
}

static error_t sdl_display_draw_bitmap(Device* device, int32_t x_start, int32_t y_start, int32_t x_end, int32_t y_end, const void* color_data) {
    auto* internal = static_cast<SdlDisplayInternal*>(device_get_driver_data(device));

    if (internal->init_failed) {
        return ERROR_RESOURCE;
    }

    if (!internal->initialized) {
        if (!sdl_display_lazy_init(device, internal)) {
            internal->init_failed = true;
            return ERROR_RESOURCE;
        }
        internal->initialized = true;
    }

    SDL_Rect rect = { x_start, y_start, x_end - x_start, y_end - y_start };
    // RGB565 = 2 bytes/pixel.
    if (SDL_UpdateTexture(internal->texture, &rect, color_data, (x_end - x_start) * 2) != 0) {
        return ERROR_RESOURCE;
    }

    sdl_display_present_now();
    return ERROR_NONE;
}

static enum DisplayColorFormat sdl_display_get_color_format(Device*) {
    return DISPLAY_COLOR_FORMAT_RGB565;
}

static uint16_t sdl_display_get_resolution_x(Device* device) {
    return GET_CONFIG(device)->horizontal_resolution;
}

static uint16_t sdl_display_get_resolution_y(Device* device) {
    return GET_CONFIG(device)->vertical_resolution;
}

// endregion

static const DisplayApi sdl_display_api = {
    .capabilities = 0,
    .reset = sdl_display_reset,
    .init = sdl_display_init,
    .draw_bitmap = sdl_display_draw_bitmap,
    .clear = nullptr,
    .refresh = nullptr,
    .mirror = nullptr,
    .swap_xy = nullptr,
    .get_swap_xy = nullptr,
    .get_mirror_x = nullptr,
    .get_mirror_y = nullptr,
    .set_gap = nullptr,
    .get_gap_x = nullptr,
    .get_gap_y = nullptr,
    .invert_color = nullptr,
    .disp_on_off = nullptr,
    .disp_sleep = nullptr,
    .get_color_format = sdl_display_get_color_format,
    .get_resolution_x = sdl_display_get_resolution_x,
    .get_resolution_y = sdl_display_get_resolution_y,
    .get_frame_buffer = nullptr,
    .get_frame_buffer_count = nullptr,
    .get_backlight = nullptr,
    .has_capability = nullptr,
};

extern Module websimulator_module;

Driver sdl_display_driver = {
    .name = "sdl-display",
    .compatible = (const char*[]) { "tactility,sdl-display", nullptr },
    .start_device = start,
    .stop_device = stop,
    .probe = nullptr,
    .api = &sdl_display_api,
    .device_type = &DISPLAY_TYPE,
    .internal = nullptr,
};
