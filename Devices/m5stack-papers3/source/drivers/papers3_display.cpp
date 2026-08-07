// SPDX-License-Identifier: Apache-2.0
#include "papers3_display.h"

#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/drivers/display.h>
#include <tactility/error.h>
#include <tactility/log.h>
#include <tactility/module.h>

#include <epd_board.h>
#include <epdiy.h>

#include <esp_heap_caps.h>

#include <cstdlib>
#include <cstring>

#define TAG "Papers3Display"
#define GET_CONFIG(device) (static_cast<const Papers3DisplayConfig*>((device)->config))

extern "C" {

extern Module m5stack_papers3_module;

// epd_hl_init() has no matching deinit and sets an internal already_initialized flag, so the
// highlevel state must persist across stop()/start() cycles and be reused rather than recreated.
static bool s_hl_initialized = false;
static EpdiyHighlevelState s_hl_state = {};

struct Papers3DisplayInternal {
    EpdiyHighlevelState hl_state;
    uint8_t* framebuffer;
    // Scratch buffer for the grayscale8->EPDiy(4bpp packed, 2px/byte) conversion in draw_bitmap().
    uint8_t* packed_buffer;
    bool powered;
};

static void power_on(Papers3DisplayInternal* internal) {
    if (!internal->powered) {
        epd_poweron();
        internal->powered = true;
    }
}

// region DisplayApi

static error_t papers3_display_reset(Device* device) {
    auto* internal = static_cast<Papers3DisplayInternal*>(device_get_driver_data(device));
    // EPD has no discrete reset pin/sequence the way SPI TFT panels do - epd_init() (in start())
    // already performs the real one-time hardware bring-up. A power-cycle is the closest
    // equivalent available at runtime.
    epd_poweroff();
    internal->powered = false;
    power_on(internal);
    return ERROR_NONE;
}

static error_t papers3_display_init(Device* device) {
    const auto* config = GET_CONFIG(device);
    auto* internal = static_cast<Papers3DisplayInternal*>(device_get_driver_data(device));
    power_on(internal);
    // The bootloader/boot-logo splash draws via partial refreshes that never get a real quality
    // pass, leaving a faint ghost. Run a full clear now, before LVGL's first flush ever reaches
    // draw_bitmap(), so it never has to undo content LVGL already put on screen.
    epd_fullclear(&internal->hl_state, config->temperature_celsius);
    return ERROR_NONE;
}

// Reports GRAYSCALE8 (not MONOCHROME) so LVGL uses partial/tile updates instead of forcing
// full-frame - the bridge hardcodes full-frame for MONOCHROME/I1 regardless of capability flags.
// So draw_bitmap is called once per changed tile, not necessarily the whole panel.
static error_t papers3_display_draw_bitmap(Device* device, int32_t x_start, int32_t y_start, int32_t x_end, int32_t y_end, const void* color_data) {
    auto* internal = static_cast<Papers3DisplayInternal*>(device_get_driver_data(device));
    const auto* config = GET_CONFIG(device);

    const int32_t width = x_end - x_start;
    const int32_t height = y_end - y_start;

    // color_data is DISPLAY_COLOR_FORMAT_GRAYSCALE8: row-major, 1 byte/pixel luminance
    // (0x00=black..0xFF=white, matching LVGL's L8). EPDiy wants 4bpp packed (2px/byte, 0x0=black,
    // 0xF=white) - a plain >>4 truncation preserves all 16 real gray levels the panel supports
    // (this panel is not B/W-only; see MODE_GC16/GL16 in papers3-display.yaml's draw-mode doc).
    const auto* src = static_cast<const uint8_t*>(color_data);
    const size_t src_stride = static_cast<size_t>(width);
    const size_t packed_stride = static_cast<size_t>(width + 1) / 2;

    for (int32_t row = 0; row < height; row++) {
        const uint8_t* src_row = src + static_cast<size_t>(row) * src_stride;
        uint8_t* dst_row = internal->packed_buffer + static_cast<size_t>(row) * packed_stride;
        int32_t col = 0;
        for (; col + 2 <= width; col += 2) {
            const uint8_t p0 = src_row[col] >> 4U;
            const uint8_t p1 = src_row[col + 1] >> 4U;
            dst_row[col / 2] = static_cast<uint8_t>((p1 << 4U) | p0);
        }
        if (col < width) { // odd width: last column has no pair, low nibble unused
            dst_row[col / 2] = static_cast<uint8_t>(src_row[col] >> 4U);
        }
    }

    const EpdRect update_area = {
        .x = x_start,
        .y = y_start,
        .width = static_cast<uint16_t>(width),
        .height = static_cast<uint16_t>(height)
    };

    power_on(internal);
    epd_draw_rotated_image(update_area, internal->packed_buffer, internal->framebuffer);
    auto draw_result = epd_hl_update_area(
        &internal->hl_state,
        static_cast<EpdDrawMode>(config->draw_mode | MODE_PACKING_2PPB),
        config->temperature_celsius,
        update_area
    );

    return draw_result == EPD_DRAW_SUCCESS ? ERROR_NONE : ERROR_RESOURCE;
}

static error_t papers3_display_disp_on_off(Device* device, bool on_off) {
    auto* internal = static_cast<Papers3DisplayInternal*>(device_get_driver_data(device));
    if (on_off) {
        power_on(internal);
    } else if (internal->powered) {
        epd_poweroff();
        internal->powered = false;
    }
    return ERROR_NONE;
}

static DisplayColorFormat papers3_display_get_color_format(Device*) {
    return DISPLAY_COLOR_FORMAT_GRAYSCALE8;
}

// epd_width()/epd_height() are the panel's native, unrotated dimensions (display->width/height in
// epdiy.c) - epd_rotated_display_width()/height() swap them for EPD_ROT_PORTRAIT/INVERTED_PORTRAIT.
// epd_draw_rotated_image() clamps its input rect against the *rotated* dims and epd_draw_pixel()
// applies the rotation transform on top of that (see _rotate() in epdiy.c), so both LVGL's canvas
// size and draw_bitmap()'s rect must be in rotated-space, not native-space - using the native
// epd_width()/epd_height() here fed rotated-space code a landscape-sized canvas, which produced
// exactly the "rotated + landscape" symptom this was fixed for.
static uint16_t papers3_display_get_resolution_x(Device*) {
    return static_cast<uint16_t>(epd_rotated_display_width());
}

static uint16_t papers3_display_get_resolution_y(Device*) {
    return static_cast<uint16_t>(epd_rotated_display_height());
}

static void papers3_display_get_frame_buffer(Device*, uint8_t, void** out_buffer) {
    // Not exposed via the generic fb-direct path: EPDiy's framebuffer is its own 4bpp packed
    // format, not the DISPLAY_COLOR_FORMAT_GRAYSCALE8 (1 byte/pixel) this driver reports - see
    // get_frame_buffer_count() and draw_bitmap()'s conversion.
    *out_buffer = nullptr;
}

static uint8_t papers3_display_get_frame_buffer_count(Device*) {
    return 0;
}

// endregion

static const DisplayApi papers3_display_api = {
    // PREFER_EXTERNAL_RAM: draw_bitmap() converts into packed_buffer before touching hardware,
    // never DMAs from LVGL's pointer directly - frees LVGL's draw buffers from forced internal RAM.
    .capabilities = DISPLAY_CAPABILITY_ON_OFF | DISPLAY_CAPABILITY_SLOW_REFRESH | DISPLAY_CAPABILITY_PREFER_EXTERNAL_RAM,
    .reset = papers3_display_reset,
    .init = papers3_display_init,
    .draw_bitmap = papers3_display_draw_bitmap,
    .mirror = nullptr,
    .swap_xy = nullptr,
    .get_swap_xy = nullptr,
    .get_mirror_x = nullptr,
    .get_mirror_y = nullptr,
    .set_gap = nullptr,
    .get_gap_x = nullptr,
    .get_gap_y = nullptr,
    .invert_color = nullptr,
    .disp_on_off = papers3_display_disp_on_off,
    .disp_sleep = nullptr,
    .get_color_format = papers3_display_get_color_format,
    .get_resolution_x = papers3_display_get_resolution_x,
    .get_resolution_y = papers3_display_get_resolution_y,
    .get_frame_buffer = papers3_display_get_frame_buffer,
    .get_frame_buffer_count = papers3_display_get_frame_buffer_count,
    .get_backlight = nullptr,
    .has_capability = nullptr,
};

// region Driver lifecycle

static error_t start(Device* device) {
    const auto* config = GET_CONFIG(device);

    auto* internal = static_cast<Papers3DisplayInternal*>(malloc(sizeof(Papers3DisplayInternal)));
    if (internal == nullptr) {
        return ERROR_OUT_OF_MEMORY;
    }
    internal->powered = false;

    epd_init(&epd_board_m5papers3, &ED047TC1, static_cast<EpdInitOptions>(EPD_LUT_1K | EPD_FEED_QUEUE_32));
    epd_set_rotation(config->rotation);

    if (!s_hl_initialized) {
        s_hl_state = epd_hl_init(EPD_BUILTIN_WAVEFORM);
        if (s_hl_state.front_fb == nullptr) {
            LOG_E(TAG, "Failed to initialize EPDiy highlevel state");
            epd_deinit();
            free(internal);
            return ERROR_RESOURCE;
        }
        s_hl_initialized = true;
    } else {
        LOG_I(TAG, "Reusing existing EPDiy highlevel state");
    }

    internal->hl_state = s_hl_state;
    internal->framebuffer = epd_hl_get_framebuffer(&internal->hl_state);

    // Sized for the rotated (LVGL-facing) resolution - see get_resolution_x()/y()'s comment.
    // ~260KB for this panel - a plain malloc() would land in scarce internal RAM. This buffer is
    // only ever read once per draw_bitmap() call by epd_draw_rotated_image() (into epdiy's own
    // SPIRAM-backed framebuffers, see highlevel.c), so it has no internal-RAM/DMA requirement and
    // belongs in PSRAM instead, matching epdiy's own front_fb/back_fb/difference_fb allocations.
    const size_t packed_buffer_size = static_cast<size_t>((epd_rotated_display_width() + 1) / 2) * static_cast<size_t>(epd_rotated_display_height());
    internal->packed_buffer = static_cast<uint8_t*>(heap_caps_malloc(packed_buffer_size, MALLOC_CAP_SPIRAM));
    if (internal->packed_buffer == nullptr) {
        LOG_E(TAG, "Failed to allocate packed pixel buffer");
        epd_deinit();
        free(internal);
        return ERROR_OUT_OF_MEMORY;
    }

    device_set_driver_data(device, internal);

    LOG_I(TAG, "EPDiy initialized (%dx%d native, %dx%d rotated)", epd_width(), epd_height(), epd_rotated_display_width(), epd_rotated_display_height());
    return ERROR_NONE;
}

static error_t stop(Device* device) {
    auto* internal = static_cast<Papers3DisplayInternal*>(device_get_driver_data(device));

    if (internal->powered) {
        epd_poweroff();
        internal->powered = false;
    }

    epd_deinit();

    free(internal->packed_buffer);
    free(internal);
    device_set_driver_data(device, nullptr);
    return ERROR_NONE;
}

// endregion

Driver papers3_display_driver = {
    .name = "papers3-display",
    .compatible = (const char*[]) { "m5stack,papers3-display", nullptr },
    .start_device = start,
    .stop_device = stop,
    .api = &papers3_display_api,
    .device_type = &DISPLAY_TYPE,
    .owner = &m5stack_papers3_module,
    .internal = nullptr
};

}
