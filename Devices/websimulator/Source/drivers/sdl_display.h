// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <SDL2/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct SdlDisplayConfig {
    uint16_t horizontal_resolution;
    uint16_t vertical_resolution;
};

/**
 * @return the display's renderer, or NULL if the display hasn't drawn its first frame yet.
 *     Used by sdl_input.cpp to map window coordinates back to the fixed logical resolution
 *     the renderer scales to fit the canvas.
 */
SDL_Renderer* sdl_display_get_renderer(void);

/**
 * @brief Re-presents the already-drawn frame at the renderer's current scale after a canvas
 *     resize. No-op if the display hasn't drawn its first frame yet.
 */
void sdl_display_present_now(void);

#ifdef __cplusplus
}
#endif
