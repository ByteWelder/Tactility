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
 * @return the display's renderer, or NULL if the display hasn't drawn its first frame yet
 *     (see sdl_display_lazy_init() in sdl_display.cpp). Used by sdl_input.cpp to map window
 *     coordinates back to the fixed logical resolution the renderer scales to fit the window.
 */
SDL_Renderer* sdl_display_get_renderer(void);

/**
 * @brief Re-presents the already-drawn frame at the renderer's current scale. Call this when the
 *     window is resized: the window size change alone doesn't make SDL re-blit the last frame at
 *     the new scale until something calls SDL_RenderPresent() again, and LVGL won't do that on
 *     its own since nothing it's tracking actually changed. No-op if the display hasn't drawn its
 *     first frame yet.
 */
void sdl_display_present_now(void);

#ifdef __cplusplus
}
#endif
