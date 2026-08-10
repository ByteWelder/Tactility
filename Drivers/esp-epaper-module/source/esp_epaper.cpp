// SPDX-License-Identifier: Apache-2.0
#include <drivers/esp_epaper.h>
#include <esp_epaper_module.h>
#include "esp_epaper_rotate.h"

#include <tactility/check.h>
#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/drivers/display.h>
#include <tactility/drivers/esp32_spi.h>
#include <tactility/drivers/spi_controller.h>
#include <tactility/error.h>
#include <tactility/log.h>

#include <epaper.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <cstdlib>
#include <cstring>

constexpr auto* TAG = "esp_epaper";
#define GET_CONFIG(device) (static_cast<const EspEpaperConfig*>((device)->config))

/** Width/height overrides above this are rejected as nonsense. */
constexpr uint16_t MAX_PANEL_DIMENSION = 2048;

// Windowed partial refresh memory budget: the replay buffer holds every window
// written to 0x24 in one refresh cycle, so commit_base() can mirror them into
// the base plane (0x26) once the panel finished driving. A cycle whose tiles
// exceed this budget escalates to a full refresh instead. Together with LVGL's
// ~4-5KB draw buffer this replaces the old 48KB full-frame path on the X4.
constexpr uint32_t ESP_EPAPER_REPLAY_BYTES = 8192;
// The replay buffer is stored as back-to-back tiles; this bounds the tile
// metadata array. Escalates to a full refresh when a cycle needs more tiles
// than fit, which only happens for pathological many-window cycles.
constexpr uint16_t ESP_EPAPER_REPLAY_MAX_TILES = 64;
// Repeated differential (0xFC) refreshes against the same base image accumulate
// ghosting, so every Nth partial cycle the whole panel is refreshed instead.
constexpr uint32_t ESP_EPAPER_PARTIALS_BEFORE_FULL = 10;

// One windowed tile stored in the replay buffer, in the order draw_bitmap()
// streamed it to 0x24 during the current refresh cycle.
struct EspEpaperReplayTile {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    uint32_t data_offset;
};

struct EspEpaperInternal {
    /** Opaque esp_epaper device, owns the panel's pins and SPI device. */
    epd_handle_t epd;
    epd_panel_info_t info;
    /** Scratch buffer in native panel layout, for rotated frames (rotation != 0). */
    uint8_t* rotate_buffer;
    /** Serializes panel/SPI access between draw_bitmap and power state changes. */
    SemaphoreHandle_t panel_mutex;
    /** disp_on_off state; the panel is in deep sleep while false. */
    bool display_on;
    // Windowed partial refresh state (use_partial only): LVGL streams byte-aligned
    // tiles into the panel RAM (0x24) during a cycle, then refresh() triggers the
    // panel drive, and commit_base() replays the cycle's tiles into 0x26 so the
    // differential refresh sequence keeps diffling against the frame on screen.
    bool use_partial;
    bool cycle_has_tiles;
    bool cycle_overflowed;
    // First cycle after start/wake must be full: deep sleep clears GDDRAM while
    // the retained image stays on screen, so a differential refresh would be
    // meaningless until the panel has driven the new frame once.
    bool force_full;
    uint32_t partial_count;
    uint8_t* replay;
    uint32_t replay_len;
    uint16_t replay_tile_count;
    EspEpaperReplayTile replay_tiles[ESP_EPAPER_REPLAY_MAX_TILES];
};

static uint16_t esp_epaper_get_display_width(const EspEpaperInternal* internal, uint8_t rotation) {
    return esp_epaper_rotation_swaps_axes(rotation) ? internal->info.height : internal->info.width;
}

static uint16_t esp_epaper_get_display_height(const EspEpaperInternal* internal, uint8_t rotation) {
    return esp_epaper_rotation_swaps_axes(rotation) ? internal->info.width : internal->info.height;
}

static bool resolve_panel_type(const char* name, epd_panel_type_t* out_type) {
    struct PanelMapping {
        const char* name;
        epd_panel_type_t type;
    };
    static constexpr PanelMapping kPanelMappings[] = {
        { "gdey0154d67", EPD_PANEL_GDEY0154D67 },
        { "gdep073e01", EPD_PANEL_GDEP073E01 },
        { "gdey037f51", EPD_PANEL_GDEY037F51 },
        { "gdey029t71h", EPD_PANEL_GDEY029T71H },
        { "gdeq0426t82", EPD_PANEL_GDEQ0426T82 },
        { "ssd16xx-154", EPD_PANEL_SSD16XX_154 },
        { "ssd16xx-213", EPD_PANEL_SSD16XX_213 },
        { "ssd16xx-266", EPD_PANEL_SSD16XX_266 },
        { "ssd16xx-270", EPD_PANEL_SSD16XX_270 },
        { "ssd16xx-290", EPD_PANEL_SSD16XX_290 },
        { "ssd16xx-370", EPD_PANEL_SSD16XX_370 },
        { "ssd16xx-420", EPD_PANEL_SSD16XX_420 },
    };
    for (const auto& mapping : kPanelMappings) {
        if (std::strcmp(name, mapping.name) == 0) {
            *out_type = mapping.type;
            return true;
        }
    }
    return false;
}

static error_t esp_epaper_reset(Device* device) {
    auto* internal = static_cast<EspEpaperInternal*>(device_get_driver_data(device));
    xSemaphoreTake(internal->panel_mutex, portMAX_DELAY);
    // epd_wake() toggles the reset pin and re-runs the full init sequence.
    const esp_err_t ret = epd_wake(internal->epd);
    // epd_wake() re-inits the panel, so it is awake (and drawable) again.
    if (ret == ESP_OK) {
        internal->display_on = true;
        // The re-init cleared GDDRAM, so the first refresh after reset must be full.
        internal->force_full = true;
    }
    xSemaphoreGive(internal->panel_mutex);
    return ret == ESP_OK ? ERROR_NONE : ERROR_RESOURCE;
}

static error_t esp_epaper_init(Device* device) {
    auto* internal = static_cast<EspEpaperInternal*>(device_get_driver_data(device));
    xSemaphoreTake(internal->panel_mutex, portMAX_DELAY);
    const esp_err_t ret = epd_wake(internal->epd);
    if (ret == ESP_OK) {
        internal->display_on = true;
        internal->force_full = true;
    }
    xSemaphoreGive(internal->panel_mutex);
    return ret == ESP_OK ? ERROR_NONE : ERROR_RESOURCE;
}

// Windowed partial draw (use_partial only): streams one byte-aligned tile into
// the display RAM (0x24) and records it for the base-plane commit. LVGL's I1
// areas are always byte-aligned in X (lv_refr.c rounds them), so the tile maps
// 1:1 onto a 0x24 window. Tiles accumulate into the replay buffer; a cycle that
// would exceed the replay budget is escalated to a full refresh instead - this
// is what keeps the partial path's extra RAM fixed at ESP_EPAPER_REPLAY_BYTES
// regardless of how much of the panel a cycle touches.
static error_t esp_epaper_draw_bitmap_partial(Device* device, int32_t x_start, int32_t y_start, int32_t x_end, int32_t y_end, const void* color_data) {
    auto* internal = static_cast<EspEpaperInternal*>(device_get_driver_data(device));

    if ((x_start & 7) != 0 || (x_end & 7) != 0) {
        LOG_W(TAG, "partial draw_bitmap: x range %ld..%ld is not byte-aligned", (long)x_start, (long)x_end);
        return ERROR_NOT_SUPPORTED;
    }
    const uint32_t width_bytes = (uint32_t)(x_end - x_start) / 8;
    const uint32_t height = (uint32_t)(y_end - y_start);
    const uint32_t tile_bytes = width_bytes * height;
    if (tile_bytes == 0) {
        return ERROR_NONE;
    }

    const auto* data = static_cast<const uint8_t*>(color_data);
    const uint16_t x = (uint16_t)x_start;
    const uint16_t y = (uint16_t)y_start;
    const uint16_t w = (uint16_t)(x_end - x_start);
    const uint16_t h = (uint16_t)(y_end - y_start);

    xSemaphoreTake(internal->panel_mutex, portMAX_DELAY);

    if (!internal->display_on) {
        // Display is off (deep sleep). Drop the tile; the next power-on forces a
        // full refresh of the current render anyway.
        xSemaphoreGive(internal->panel_mutex);
        return ERROR_NONE;
    }

    const bool overflow = internal->cycle_overflowed ||
        internal->replay_tile_count >= ESP_EPAPER_REPLAY_MAX_TILES ||
        internal->replay_len + tile_bytes > ESP_EPAPER_REPLAY_BYTES;

    esp_err_t ret;
    if (overflow) {
        // Escalate this and every remaining tile of the cycle to a full refresh:
        // stream each window straight into both planes, so 0x26 needs no replay
        // later and 0xF7 drives the accumulated frame.
        if (!internal->cycle_overflowed) {
            internal->cycle_overflowed = true;
            LOG_I(TAG, "Partial cycle exceeds replay budget (%u bytes), escalating to full refresh", (unsigned)ESP_EPAPER_REPLAY_BYTES);
        }
        ret = epd_write_partial(internal->epd, x, y, w, h, data);
        if (ret == ESP_OK) {
            ret = epd_write_base_partial(internal->epd, x, y, w, h, data);
        }
    } else {
        auto& tile = internal->replay_tiles[internal->replay_tile_count];
        tile.x = x;
        tile.y = y;
        tile.w = w;
        tile.h = h;
        tile.data_offset = internal->replay_len;
        memcpy(internal->replay + internal->replay_len, data, tile_bytes);
        internal->replay_len += tile_bytes;
        internal->replay_tile_count++;
        ret = epd_write_partial(internal->epd, x, y, w, h, data);
    }

    xSemaphoreGive(internal->panel_mutex);
    if (ret != ESP_OK) {
        LOG_E(TAG, "epd_write_partial failed: %s", esp_err_to_name(ret));
        return ERROR_RESOURCE;
    }
    internal->cycle_has_tiles = true;
    return ERROR_NONE;
}

// LVGL only ever calls this with the full frame: DISPLAY_COLOR_FORMAT_MONOCHROME forces
// LV_DISPLAY_RENDER_MODE_FULL in the generic kernel LVGL bridge (lvgl_display.c), and FULL mode
// only presents (calls draw_bitmap) once per render cycle, with the complete 0,0..hres,vres rect.
// hres/vres are the rotated (display) resolution reported by get_resolution_*; color_data is
// row-major, MSB-first 1bpp (LVGL's LV_COLOR_FORMAT_I1 with the palette header already stripped by
// the caller); bit 1 = white, bit 0 = black. epd_update_async() expects exactly that layout and
// polarity. It streams color_data into GDDRAM and fires the master activation, then returns while
// the panel keeps driving; the busy wait for the refresh moved to esp_epaper_wait_sync() so the
// render thread no longer blocks for the whole refresh. color_data is only read during the
// synchronous SPI transfer, so LVGL's buffer is free the moment this returns.
static error_t esp_epaper_draw_bitmap(Device* device, int32_t x_start, int32_t y_start, int32_t x_end, int32_t y_end, const void* color_data) {
    auto* internal = static_cast<EspEpaperInternal*>(device_get_driver_data(device));
    const auto* config = GET_CONFIG(device);

    if (internal->use_partial) {
        return esp_epaper_draw_bitmap_partial(device, x_start, y_start, x_end, y_end, color_data);
    }

    const uint16_t display_width = esp_epaper_get_display_width(internal, config->rotation);
    const uint16_t display_height = esp_epaper_get_display_height(internal, config->rotation);
    if (x_start != 0 || y_start != 0 || x_end != display_width || y_end != display_height) {
        LOG_W(TAG, "draw_bitmap: only full-frame draws are supported (got %ld,%ld..%ld,%ld)", (long)x_start, (long)y_start, (long)x_end, (long)y_end);
        return ERROR_NOT_SUPPORTED;
    }

    xSemaphoreTake(internal->panel_mutex, portMAX_DELAY);

    if (!internal->display_on) {
        // Display is off (deep sleep). Drop the frame; the mismatch is fine because the next
        // power-on triggers a full refresh of the current render anyway.
        xSemaphoreGive(internal->panel_mutex);
        return ERROR_NONE;
    }

    const uint8_t* source;
    if (config->rotation == 0) {
        source = static_cast<const uint8_t*>(color_data);
    } else {
        memset(internal->rotate_buffer, 0, internal->info.buffer_size);
        esp_epaper_rotate_frame(static_cast<const uint8_t*>(color_data), internal->rotate_buffer,
                                internal->info.width, internal->info.height, config->rotation);
        source = internal->rotate_buffer;
    }

    const esp_err_t ret = epd_update_async(internal->epd, source, config->update_mode);
    xSemaphoreGive(internal->panel_mutex);
    if (ret != ESP_OK) {
        LOG_E(TAG, "epd_update_async failed: %s", esp_err_to_name(ret));
        return ERROR_RESOURCE;
    }
    return ERROR_NONE;
}

// Trigger the panel drive for the tiles streamed during this refresh cycle
// (use_partial only). Partial unless the caller asked for full, the first cycle
// after a wake/reset (force_full), the cycle overran the replay budget, or the
// periodic ghosting-cleanup counter is due. Returns immediately - the panel
// keeps driving in the background until wait_sync().
static error_t esp_epaper_refresh(Device* device, bool full_frame) {
    auto* internal = static_cast<EspEpaperInternal*>(device_get_driver_data(device));

    xSemaphoreTake(internal->panel_mutex, portMAX_DELAY);
    if (!internal->cycle_has_tiles) {
        // Nothing was drawn this cycle (dropped frame or display off); nothing to drive.
        xSemaphoreGive(internal->panel_mutex);
        return ERROR_NONE;
    }

    bool do_full = full_frame || internal->force_full || internal->cycle_overflowed;
    if (do_full) {
        internal->partial_count = 0;
    } else {
        internal->partial_count++;
        if (internal->partial_count >= ESP_EPAPER_PARTIALS_BEFORE_FULL) {
            internal->partial_count = 0;
            do_full = true;
        }
    }
    internal->force_full = false;

    const epd_update_mode_t mode = do_full ? EPD_UPDATE_FULL : EPD_UPDATE_PARTIAL;
    const esp_err_t ret = epd_update_partial_async(internal->epd, mode);

    // The cycle is consumed. The replay tiles are kept for commit_base() - which
    // runs after the panel finishes driving - unless the trigger failed, in which
    // case the panel stays idle and the next cycle must start from a clean slate.
    internal->cycle_has_tiles = false;
    internal->cycle_overflowed = false;
    if (ret != ESP_OK) {
        internal->replay_len = 0;
        internal->replay_tile_count = 0;
    }

    xSemaphoreGive(internal->panel_mutex);
    if (ret != ESP_OK) {
        LOG_E(TAG, "epd_update_partial_async (%s) failed: %s", do_full ? "full" : "partial", esp_err_to_name(ret));
        return ERROR_RESOURCE;
    }
    return ERROR_NONE;
}

// Mirror the last refresh cycle's windows into the base image RAM (0x26)
// (use_partial only). Must run after the panel finished driving (wait_sync()):
// 0x26 is the frame the differential 0xFC refresh diffs against, so it must
// always equal what is actually on screen. Runs on the bridge's refresh task,
// never on the render thread.
static error_t esp_epaper_commit_base(Device* device) {
    auto* internal = static_cast<EspEpaperInternal*>(device_get_driver_data(device));

    xSemaphoreTake(internal->panel_mutex, portMAX_DELAY);
    if (internal->replay_tile_count == 0) {
        xSemaphoreGive(internal->panel_mutex);
        return ERROR_NONE;
    }

    esp_err_t ret = ESP_OK;
    for (uint16_t i = 0; i < internal->replay_tile_count; ++i) {
        const auto& tile = internal->replay_tiles[i];
        ret = epd_write_base_partial(internal->epd, tile.x, tile.y, tile.w, tile.h,
                                     internal->replay + tile.data_offset);
        if (ret != ESP_OK) {
            LOG_E(TAG, "epd_write_base_partial (%u,%u %ux%u) failed: %s",
                  (unsigned)tile.x, (unsigned)tile.y, (unsigned)tile.w, (unsigned)tile.h, esp_err_to_name(ret));
            break;
        }
    }

    internal->replay_len = 0;
    internal->replay_tile_count = 0;

    xSemaphoreGive(internal->panel_mutex);
    return ret == ESP_OK ? ERROR_NONE : ERROR_RESOURCE;
}

// Blocks until the panel finished the refresh triggered by the last
// esp_epaper_draw_bitmap(). Only polls the BUSY GPIO - no SPI traffic - so it
// runs without the panel mutex and cannot conflict with a concurrent stream.
static error_t esp_epaper_wait_sync(Device* device, uint32_t timeout_ms) {
    auto* internal = static_cast<EspEpaperInternal*>(device_get_driver_data(device));
    // epd_wait_busy() returns ESP_OK even when it gave up on the timeout, so
    // the still-busy state after it returns is what signals ERROR_TIMEOUT.
    const esp_err_t ret = epd_wait_busy(internal->epd, timeout_ms);
    if (ret != ESP_OK) {
        return ERROR_RESOURCE;
    }
    if (epd_is_busy(internal->epd)) {
        LOG_W(TAG, "wait_sync timeout: panel still busy after %lu ms", (unsigned long)timeout_ms);
        return ERROR_TIMEOUT;
    }
    return ERROR_NONE;
}

static error_t esp_epaper_disp_on_off(Device* device, bool on_off) {
    auto* internal = static_cast<EspEpaperInternal*>(device_get_driver_data(device));

    xSemaphoreTake(internal->panel_mutex, portMAX_DELAY);
    if (on_off == internal->display_on) {
        xSemaphoreGive(internal->panel_mutex);
        return ERROR_NONE;
    }

    bool ok = true;
    if (on_off) {
        if (epd_wake(internal->epd) != ESP_OK) {
            LOG_E(TAG, "epd_wake failed");
            ok = false;
        }
    } else {
        if (epd_sleep(internal->epd) != ESP_OK) {
            LOG_E(TAG, "epd_sleep failed");
            ok = false;
        }
    }

    if (ok) {
        internal->display_on = on_off;
        if (on_off) {
            // Deep sleep cleared GDDRAM; the first refresh after wake must be full.
            internal->force_full = true;
        }
    }
    xSemaphoreGive(internal->panel_mutex);
    return ok ? ERROR_NONE : ERROR_RESOURCE;
}

static DisplayColorFormat esp_epaper_get_color_format(Device*) {
    return DISPLAY_COLOR_FORMAT_MONOCHROME;
}

static uint16_t esp_epaper_get_resolution_x(Device* device) {
    auto* internal = static_cast<EspEpaperInternal*>(device_get_driver_data(device));
    return esp_epaper_get_display_width(internal, GET_CONFIG(device)->rotation);
}

static uint16_t esp_epaper_get_resolution_y(Device* device) {
    auto* internal = static_cast<EspEpaperInternal*>(device_get_driver_data(device));
    return esp_epaper_get_display_height(internal, GET_CONFIG(device)->rotation);
}

static void esp_epaper_get_frame_buffer(Device*, uint8_t, void** out_buffer) {
    *out_buffer = nullptr;
}

static uint8_t esp_epaper_get_frame_buffer_count(Device*) {
    return 0;
}

static bool esp_epaper_has_capability(Device* device, uint32_t capability);

static const DisplayApi esp_epaper_display_api = {
    .capabilities = DISPLAY_CAPABILITY_ON_OFF | DISPLAY_CAPABILITY_SLOW_REFRESH |
        DISPLAY_CAPABILITY_PARTIAL_REFRESH,
    .reset = esp_epaper_reset,
    .init = esp_epaper_init,
    .draw_bitmap = esp_epaper_draw_bitmap,
    .wait_sync = esp_epaper_wait_sync,
    .refresh = esp_epaper_refresh,
    .commit_base = esp_epaper_commit_base,
    .mirror = nullptr,
    .swap_xy = nullptr,
    .get_swap_xy = nullptr,
    .get_mirror_x = nullptr,
    .get_mirror_y = nullptr,
    .set_gap = nullptr,
    .get_gap_x = nullptr,
    .get_gap_y = nullptr,
    .invert_color = nullptr,
    .disp_on_off = esp_epaper_disp_on_off,
    .disp_sleep = nullptr,
    .get_color_format = esp_epaper_get_color_format,
    .get_resolution_x = esp_epaper_get_resolution_x,
    .get_resolution_y = esp_epaper_get_resolution_y,
    .get_frame_buffer = esp_epaper_get_frame_buffer,
    .get_frame_buffer_count = esp_epaper_get_frame_buffer_count,
    .get_backlight = nullptr,
    .has_capability = esp_epaper_has_capability,
};

static bool esp_epaper_has_capability(Device* device, uint32_t capability) {
    auto* internal = static_cast<EspEpaperInternal*>(device_get_driver_data(device));
    if ((capability & DISPLAY_CAPABILITY_PARTIAL_REFRESH) != 0) {
        // Windowed refresh only works in the panel's native orientation: the
        // driver handles fixed rotation != 0 via a whole-frame rotate, and LVGL
        // rotation is unsupported (the bridge drops partial cycles while rotating).
        return internal->use_partial;
    }
    return (esp_epaper_display_api.capabilities & capability) == capability;
}

static void free_internal(EspEpaperInternal* internal) {
    if (internal->epd != nullptr) {
        epd_deinit(internal->epd);
    }
    if (internal->rotate_buffer != nullptr) {
        free(internal->rotate_buffer);
    }
    if (internal->replay != nullptr) {
        free(internal->replay);
    }
    if (internal->panel_mutex != nullptr) {
        vSemaphoreDelete(internal->panel_mutex);
    }
    free(internal);
}

static error_t start(Device* device) {
    auto* parent = device_get_parent(device);
    check(device_get_type(parent) == &SPI_CONTROLLER_TYPE);

    const auto* spi_config = static_cast<const Esp32SpiConfig*>(parent->config);
    const auto* config = GET_CONFIG(device);

    epd_panel_type_t panel_type;
    if (!resolve_panel_type(config->panel_type, &panel_type)) {
        LOG_E(TAG, "Unknown panel type: %s", config->panel_type);
        return ERROR_NOT_SUPPORTED;
    }

    if (config->clock_speed_hz == 0) {
        LOG_E(TAG, "Invalid clock_speed_hz (0)");
        return ERROR_NOT_SUPPORTED;
    }

    if (config->update_mode > EPD_UPDATE_PARTIAL) {
        LOG_E(TAG, "Invalid update_mode %d (expected %d-%d)", (int)config->update_mode, (int)EPD_UPDATE_FULL, (int)EPD_UPDATE_PARTIAL);
        return ERROR_NOT_SUPPORTED;
    }

    if (config->width > MAX_PANEL_DIMENSION || config->height > MAX_PANEL_DIMENSION) {
        LOG_E(TAG, "Invalid panel size %ux%u (maximum %ux%u)", (unsigned)config->width, (unsigned)config->height, (unsigned)MAX_PANEL_DIMENSION, (unsigned)MAX_PANEL_DIMENSION);
        return ERROR_NOT_SUPPORTED;
    }

    epd_config_t epd_config = EPD_CONFIG_DEFAULT();
    epd_config.pins.busy = config->pin_busy.pin;
    epd_config.pins.rst = config->pin_reset.pin;
    epd_config.pins.dc = config->pin_dc.pin;
    epd_config.pins.cs = config->pin_cs.pin;
    epd_config.pins.sck = spi_config->pin_sclk.pin;
    epd_config.pins.mosi = spi_config->pin_mosi.pin;
    epd_config.spi.host = spi_config->host;
    epd_config.spi.speed_hz = config->clock_speed_hz;
    epd_config.panel.type = panel_type;
    epd_config.panel.width = config->width;
    epd_config.panel.height = config->height;
    // The kernel bridge owns the frame buffer and hands it to epd_update() as the
    // source buffer, so the component's internal copy is redundant. Skipping it
    // saves buffer_size bytes of RAM (48KB on the 800x480 GDEQ0426T82).
    epd_config.framebuffer_enable = false;

    epd_handle_t epd = nullptr;
    if (epd_init(&epd_config, &epd) != ESP_OK) {
        LOG_E(TAG, "epd_init failed");
        return ERROR_RESOURCE;
    }

    auto* internal = static_cast<EspEpaperInternal*>(calloc(1, sizeof(EspEpaperInternal)));
    if (internal == nullptr) {
        epd_deinit(epd);
        return ERROR_OUT_OF_MEMORY;
    }

    internal->epd = epd;
    internal->panel_mutex = xSemaphoreCreateMutex();
    if (internal->panel_mutex == nullptr) {
        free_internal(internal);
        return ERROR_OUT_OF_MEMORY;
    }

    if (epd_get_info(epd, &internal->info) != ESP_OK) {
        LOG_E(TAG, "epd_get_info failed");
        free_internal(internal);
        return ERROR_RESOURCE;
    }

    if (internal->info.color_mode != EPD_COLOR_BW) {
        LOG_E(TAG, "Panel color mode %d is not supported by the kernel display bridge (monochrome only)", internal->info.color_mode);
        free_internal(internal);
        return ERROR_NOT_SUPPORTED;
    }

    if (config->rotation > 3) {
        LOG_E(TAG, "Invalid rotation %u (expected 0-3)", config->rotation);
        free_internal(internal);
        return ERROR_NOT_SUPPORTED;
    }

    if (config->rotation != 0) {
        internal->rotate_buffer = static_cast<uint8_t*>(malloc(internal->info.buffer_size));
        if (internal->rotate_buffer == nullptr) {
            LOG_E(TAG, "Failed to allocate %lu-byte rotation buffer", internal->info.buffer_size);
            free_internal(internal);
            return ERROR_OUT_OF_MEMORY;
        }
    }

    // Windowed partial refresh needs both a windowed 0x24 writer and a base image
    // (0x26) writer; only the SSD1677 controller provides the latter, so other
    // panels fall through to the full-frame path regardless of their EPD_CAP_PARTIAL.
    internal->use_partial = config->rotation == 0 &&
        epd_supports_partial(epd) && epd_supports_base_partial(epd);
    if (internal->use_partial) {
        internal->replay = static_cast<uint8_t*>(malloc(ESP_EPAPER_REPLAY_BYTES));
        if (internal->replay == nullptr) {
            LOG_E(TAG, "Failed to allocate %lu-byte partial replay buffer", (unsigned long)ESP_EPAPER_REPLAY_BYTES);
            free_internal(internal);
            return ERROR_OUT_OF_MEMORY;
        }
        // GDDRAM was just cleared to white while the retained image is still on
        // screen; the first refresh must drive the new frame (full) before any
        // differential refresh is meaningful.
        internal->force_full = true;
    }

    internal->display_on = true;
    device_set_driver_data(device, internal);

    LOG_I(TAG, "Started %ux%u panel (buffer %lu bytes, rotation %u, partial refresh %s)",
          internal->info.width, internal->info.height, internal->info.buffer_size, config->rotation,
          internal->use_partial ? "enabled" : "disabled");
    return ERROR_NONE;
}

static error_t stop(Device* device) {
    auto* internal = static_cast<EspEpaperInternal*>(device_get_driver_data(device));

    xSemaphoreTake(internal->panel_mutex, portMAX_DELAY);
    if (internal->display_on) {
        // Leave the panel in deep sleep.
        epd_sleep(internal->epd);
        internal->display_on = false;
    }
    xSemaphoreGive(internal->panel_mutex);

    free_internal(internal);
    device_set_driver_data(device, nullptr);
    return ERROR_NONE;
}

Driver esp_epaper_driver = {
    .name = "esp_epaper",
    .compatible = (const char*[]) { "tuanpmt,esp-epaper", nullptr },
    .start_device = start,
    .stop_device = stop,
    .api = &esp_epaper_display_api,
    .device_type = &DISPLAY_TYPE,
    .owner = &esp_epaper_module,
    .internal = nullptr
};
