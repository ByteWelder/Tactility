// SPDX-License-Identifier: Apache-2.0
#include <lvgl/devices/display.h>

#include <lvgl/lvgl.h>
#include <lvgl/ppa.h>

#include <tactility/concurrent/thread.h>
#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/drivers/display.h>
#include <tactility/log.h>

#include <lvgl/devices/device_context.h>

#include <atomic>
#include <stdlib.h>

#ifdef ESP_PLATFORM
#include <esp_heap_caps.h>
#endif

constexpr auto* TAG = "lvgl_display";

// How long the refresh task waits for the panel to confirm the in-flight refresh finished. Must
// exceed the longest panel refresh - a full GDEQ0426T82 refresh measured ~4s on hardware - so
// refresh_in_flight is never cleared while the panel is still busy. A premature clear would let the
// next full frame stream into the busy panel and block the render thread in the driver's (infinite)
// write-time wait-before-use, defeating the async design. On a genuine timeout the task proceeds
// anyway and clears the flag: subsequent commands are safe via that same wait-before-use.
constexpr uint32_t LVGL_DISPLAY_WAIT_SYNC_TIMEOUT_MS = 10000;
// Upper bound for how long lvgl_display_remove() may block stopping the refresh task. The task
// always exits within LVGL_DISPLAY_WAIT_SYNC_TIMEOUT_MS plus the time to reacquire the LVGL lock:
// ulTaskNotifyTake() blocks indefinitely but the stop notification wakes it, and display_wait_sync()
// honors its timeout.
constexpr TickType_t LVGL_DISPLAY_REFRESH_TASK_JOIN_TIMEOUT = pdMS_TO_TICKS(15000);
constexpr configSTACK_DEPTH_TYPE LVGL_DISPLAY_REFRESH_TASK_STACK_SIZE = 4096;

struct LvglDisplayCtx {
    void* buf1;
    void* buf2;
    bool owns_buffers; // false when buf1/buf2 point at the device's own frame buffer(s)
    // Mirrors what lvgl_display_add() passed to lv_display_set_render_mode() - there's no
    // lv_display_get_render_mode() to query it back from LVGL, so it's cached here instead.
    lv_display_render_mode_t render_mode;
    // The device's swap_xy/mirror_x/mirror_y at bind time, queried once and treated as the LV_DISPLAY_ROTATION_0 baseline.
    bool base_swap_xy;
    bool base_mirror_x;
    bool base_mirror_y;
    // The device's configured gap at bind time, in the same LV_DISPLAY_ROTATION_0 baseline frame
    // as base_swap_xy above. set_gap() is a raw (x,y) offset applied to whatever coordinates the
    // panel is currently being drawn with - it has no idea about swap_xy, so a rotation that flips
    // swap_xy relative to this baseline must swap gap_x/gap_y too (see lvgl_display_apply_rotation()).
    int32_t base_gap_x;
    int32_t base_gap_y;
    bool has_set_gap_cap;
    // When true, rotation is done in software in the flush callback instead of via display_swap_xy()/
    // display_mirror(); rotate_buf holds the rotated pixels and is sized like buf1.
    bool sw_rotate;
    // Lazily allocated on the first flush that actually rotates (see lvgl_display_ensure_rotate_buf()):
    // sw_rotate is often set for a panel that stays in its base orientation (e.g. an I1 e-paper at
    // rotation 0), where an eager full-frame buffer would be allocated and never touched.
    void* rotate_buf;
    // Cached LvglDisplayConfig::prefer_external_ram, needed when rotate_buf is allocated lazily in
    // the flush callback (config is not available there).
    bool prefer_external_ram;
    // Lazily created on the first sw_rotate flush that needs it (see lvgl_display_rotate_tile()).
    // Stays NULL - and every rotate falls back to rotate_buf/lv_draw_sw_rotate() - when the target
    // has no PPA (lvgl_ppa_is_supported()), the color format has no PPA color mode
    // (lvgl_ppa_supports_color_format()), or creating the PPA client/buffer failed once already.
    void* ppa_handle;
    bool ppa_unavailable;
    bool ppa_eligible;
    // Size of buf1/buf2 (each) - used by lvgl_display_fb_base() to range-check which real buffer
    // (for the fb-direct case) a given color_map pointer falls into.
    size_t buf_size_bytes;
    // Cached DISPLAY_CAPABILITY_CAP_SWAP_XY/CAP_MIRROR: swap_xy()/mirror() and their getters are
    // null on drivers that don't support them, so rotation handling must not call through blindly.
    bool has_swap_xy_cap;
    bool has_mirror_cap;
    // Mirrors LvglDisplayConfig::swap_bytes: the panel is big endian while the OS is little endian,
    // so we fix it in software. In the future, the driver should probably expose endianness requirements instead.
    bool byte_swap;
    // Async refresh handling for slow-refresh panels (DISPLAY_CAPABILITY_SLOW_REFRESH, e.g. e-paper).
    // The flush callback never blocks on the panel - it streams a full frame and returns, even though
    // the panel then keeps driving for 1-3s; the wait happens on the refresh task below instead, so
    // the render thread isn't stalled. See lvgl_display_refresh_task_main() for the state machine.
    bool slow_refresh;
    struct Thread* refresh_task;
    // Written by lvgl_display_remove() to ask the refresh task to exit; read by the task itself.
    std::atomic<bool> refresh_task_stop;
    // Set only by lvgl_display_flush_cb(), right after it successfully streams a full frame; cleared
    // only by the refresh task, after display_wait_sync() confirms the panel is idle again. One
    // writer per direction, so no lock is needed. Guarantees a new draw never races an in-flight
    // panel refresh.
    std::atomic<bool> refresh_in_flight;
    // Set only by lvgl_display_flush_cb() when it drops a full frame because refresh_in_flight was
    // set; cleared only by the refresh task, which then re-renders so the newest content eventually
    // reaches the panel.
    std::atomic<bool> frame_pending;
    // Windowed partial refresh (DISPLAY_CAPABILITY_PARTIAL_REFRESH on a slow-refresh I1 e-paper):
    // draw_bitmap() tiles stream into the panel RAM during a cycle, and refresh() is triggered once
    // per cycle from the last flush. The bridge drops a whole cycle when the previous refresh is
    // still driving (tiles written into a busy panel would be ignored), then re-renders. The driver
    // applies any fixed panel rotation itself (rotating tiles into a native shadow), so this path
    // is valid for rotated panels too; only a runtime LVGL rotation drops cycles.
    bool partial_refresh;
    // True from the first flush of a cycle until its last flush; gates cycle-drop detection.
    bool cycle_active;
    // True when the current cycle was dropped at its first flush (previous refresh in flight, or
    // rotation active); every flush of the cycle is skipped and no refresh is triggered.
    bool cycle_dropped;
    // Valid for the lifetime of the display; written in lvgl_display_add(), read by the refresh task.
    struct Device* refresh_task_device;
    lv_display_t* refresh_task_display;
};

static void* lvgl_display_alloc_buffer(size_t size_bytes, bool prefer_external_ram) {
#ifdef ESP_PLATFORM
    // Must match LV_DRAW_BUF_ALIGN (can be > 4 - e.g. 64, tied to the cache line size for
    // DMA2D/PPA coherency on some targets - see sdkconfig's CONFIG_LV_DRAW_BUF_ALIGN). A buffer
    // allocated less strictly than that fails lv_display_set_buffers()'s alignment assert, which
    // is configured to LV_ASSERT_HANDLER (while(1);) rather than a clean abort - i.e. a silent hang.
    // MALLOC_CAP_DMA is scarce internal RAM - skip it for displays that don't DMA directly from
    // this buffer (see prefer_external_ram_buffer). Dropping MALLOC_CAP_DMA alone isn't enough to
    // land in PSRAM though: MALLOC_CAP_8BIT alone is still satisfied by internal RAM, so
    // MALLOC_CAP_SPIRAM must be requested explicitly (confirmed on real hardware).
    uint32_t caps = prefer_external_ram ? (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : (MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    void* buf = heap_caps_aligned_alloc(LV_DRAW_BUF_ALIGN, size_bytes, caps);
    if (buf == NULL) {
        buf = heap_caps_aligned_alloc(LV_DRAW_BUF_ALIGN, size_bytes, MALLOC_CAP_DEFAULT);
    }
    return buf;
#else
    return malloc(size_bytes);
#endif
}

static void lvgl_display_free_buffer(void* buf) {
#ifdef ESP_PLATFORM
    heap_caps_free(buf);
#else
    free(buf);
#endif
}

// Resolves the kernel-reported color format to an LVGL color format. RGB565 and BGR565 (and
// their _SWAPPED variants) all render identically as far as LVGL is concerned - it has no native
// concept of channel order, only byte order (see LvglDisplayConfig::swap_bytes for that axis).
static bool lvgl_display_map_color_format(enum DisplayColorFormat in, lv_color_format_t* out) {
    switch (in) {
        case DISPLAY_COLOR_FORMAT_RGB565:
        case DISPLAY_COLOR_FORMAT_RGB565_SWAPPED:
        case DISPLAY_COLOR_FORMAT_BGR565:
        case DISPLAY_COLOR_FORMAT_BGR565_SWAPPED:
            *out = LV_COLOR_FORMAT_RGB565;
            return true;
        case DISPLAY_COLOR_FORMAT_RGB888:
            *out = LV_COLOR_FORMAT_RGB888;
            return true;
        case DISPLAY_COLOR_FORMAT_MONOCHROME:
            // Row-major, MSB-first 1bpp (matches LV_COLOR_FORMAT_I1's raw layout once the
            // palette header is stripped, see lvgl_display_flush_cb()) - any page/tile
            // reformatting a specific panel's GDDRAM needs is that driver's own concern
            // (e.g. ssd1306_draw_bitmap()'s row-to-page transpose).
            *out = LV_COLOR_FORMAT_I1;
            return true;
        case DISPLAY_COLOR_FORMAT_GRAYSCALE8:
            // Row-major, 1 byte/pixel luminance (0x00=black, 0xFF=white) - matches LV_COLOR_FORMAT_L8
            // directly, no repacking needed. Deliberately NOT routed through the I1 branch below in
            // lvgl_display_add(): I1 is hardcoded to LV_DISPLAY_RENDER_MODE_FULL there, which is what
            // this format exists to avoid for panels that want real partial/tile updates.
            *out = LV_COLOR_FORMAT_L8;
            return true;
        default:
            return false;
    }
}

static void lvgl_display_apply_rotation(struct LvglDeviceContext* wrapper, lv_display_rotation_t rotation) {
    struct LvglDisplayCtx* ctx = (struct LvglDisplayCtx*)wrapper->context;

    // SW-rotated displays stay in their base orientation; rotation is applied per-flush instead.
    if (ctx->sw_rotate) {
        return;
    }

    bool swap_xy = ctx->base_swap_xy;
    bool mirror_x = ctx->base_mirror_x;
    bool mirror_y = ctx->base_mirror_y;

    switch (rotation) {
        case LV_DISPLAY_ROTATION_0:
            break;
        case LV_DISPLAY_ROTATION_90:
            swap_xy = !ctx->base_swap_xy;
            if (ctx->base_swap_xy) {
                mirror_x = !ctx->base_mirror_x;
            } else {
                mirror_y = !ctx->base_mirror_y;
            }
            break;
        case LV_DISPLAY_ROTATION_180:
            mirror_x = !ctx->base_mirror_x;
            mirror_y = !ctx->base_mirror_y;
            break;
        case LV_DISPLAY_ROTATION_270:
            swap_xy = !ctx->base_swap_xy;
            if (ctx->base_swap_xy) {
                mirror_y = !ctx->base_mirror_y;
            } else {
                mirror_x = !ctx->base_mirror_x;
            }
            break;
    }

    if (ctx->has_swap_xy_cap) {
        display_swap_xy(wrapper->device, swap_xy);
    }
    if (ctx->has_mirror_cap) {
        display_mirror(wrapper->device, mirror_x, mirror_y);
    }
    if (ctx->has_set_gap_cap) {
        // set_gap() takes its (x,y) in whatever axes the panel is currently drawn with, not the
        // baseline's - swap gap_x/gap_y whenever this rotation's swap_xy differs from the baseline.
        bool gap_axes_swapped = swap_xy != ctx->base_swap_xy;
        int32_t gap_x = gap_axes_swapped ? ctx->base_gap_y : ctx->base_gap_x;
        int32_t gap_y = gap_axes_swapped ? ctx->base_gap_x : ctx->base_gap_y;
        display_set_gap(wrapper->device, gap_x, gap_y);
    }
}

static void lvgl_display_rotation_event_cb(lv_event_t* e) {
    struct LvglDeviceContext* wrapper = (struct LvglDeviceContext*)lv_event_get_user_data(e);
    lv_display_t* disp = (lv_display_t*)lv_event_get_current_target(e);
    lvgl_display_apply_rotation(wrapper, lv_display_get_rotation(disp));
}

// Returns which of buf1/buf2 (the real frame buffers, when !owns_buffers) color_map falls inside.
// Defaults to buf1, which also covers the single-frame-buffer (buf2 == NULL) case.
static void* lvgl_display_fb_base(struct LvglDisplayCtx* ctx, const uint8_t* color_map) {
    if (ctx->buf2 != NULL && color_map >= (uint8_t*)ctx->buf2 &&
        color_map < (uint8_t*)ctx->buf2 + ctx->buf_size_bytes) {
        return ctx->buf2;
    }
    return ctx->buf1;
}

// Tries to rotate the tightly-packed w x h block at in_buff via PPA, returning the PPA output
// buffer on success or NULL if this tile/format/target can't use it - in which case the caller
// must fall back to lv_draw_sw_rotate() into ctx->rotate_buf. Lazily creates the PPA client on the
// first eligible call, sized to ctx->buf_size_bytes (the largest tile or full-frame buffer this
// display will ever flush - see lvgl_display_add()); once creation fails once, ppa_unavailable
// latches so later tiles don't retry it. Never asks PPA to byte-swap: lvgl_display_flush_cb()
// applies ctx->byte_swap itself, once, at a single point regardless of which rotation path ran -
// simpler than tracking whether it was already done by PPA (PARTIAL mode) vs. already done
// per-tile before FULL mode's whole-frame rotate (see the two call sites).
static void* lvgl_display_try_ppa_rotate(struct LvglDisplayCtx* ctx, const uint8_t* in_buff, int32_t w, int32_t h,
                                          lv_display_rotation_t rotation, lv_color_format_t color_format) {
    if (!ctx->ppa_eligible || ctx->ppa_unavailable) {
        return NULL;
    }
    // PPA reads pic_w/pic_h in pixels with no separate stride - only safe when the tile has no
    // row padding beyond w * bytes-per-pixel (see lvgl_ppa.h).
    uint32_t bpp = lv_color_format_get_size(color_format);
    if (lv_draw_buf_width_to_stride(w, color_format) != (uint32_t)w * bpp) {
        return NULL;
    }
    if (ctx->ppa_handle == NULL) {
        ctx->ppa_handle = lvgl_ppa_get_or_create(ctx->buf_size_bytes);
        if (ctx->ppa_handle == NULL) {
            ctx->ppa_unavailable = true;
            return NULL;
        }
    }
    return lvgl_ppa_rotate(ctx->ppa_handle, in_buff, w, h, rotation, color_format, false);
}

// Ensures ctx->rotate_buf exists, allocating it sized like buf1 on the first call. sw_rotate is
// configured eagerly, but a panel may sit in its base orientation (rotation 0) for its whole life -
// e.g. an I1 e-paper - so the buffer is only created when a flush actually needs to rotate.
// Returns ERROR_NONE when the buffer is available (already or freshly allocated), ERROR_OUT_OF_MEMORY
// otherwise with rotate_buf left NULL.
static error_t lvgl_display_ensure_rotate_buf(struct LvglDisplayCtx* ctx) {
    if (ctx->rotate_buf == NULL) {
        ctx->rotate_buf = lvgl_display_alloc_buffer(ctx->buf_size_bytes, ctx->prefer_external_ram);
    }
    return ctx->rotate_buf != NULL ? ERROR_NONE : ERROR_OUT_OF_MEMORY;
}

// Owns "wait for the panel to finish a refresh" for slow-refresh panels. Woken by
// lvgl_display_flush_cb() after every successfully streamed full frame, blocks in
// display_wait_sync() until the panel is idle again, then re-renders if a frame was dropped in the
// meantime. Runs at low priority so it never competes with rendering. The stop flag is latched by
// the notification below it, so a stop request always wakes the task out of ulTaskNotifyTake().
static int32_t lvgl_display_refresh_task_main(void* context) {
    struct LvglDisplayCtx* ctx = static_cast<struct LvglDisplayCtx*>(context);

    while (!ctx->refresh_task_stop.load()) {
        // Block until a full frame was just streamed to the panel (or we're asked to stop). The
        // notification value latches, so a stream that races us re-blocking isn't lost.
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (ctx->refresh_task_stop.load()) {
            break;
        }

        // Wait for the panel to finish pushing out the frame that woke us. On timeout or a driver
        // error, proceed anyway: a later draw's write-time wait-before-use is the backstop, and
        // clearing refresh_in_flight below keeps the pipeline from stalling permanently.
        error_t wait_result = display_wait_sync(ctx->refresh_task_device, LVGL_DISPLAY_WAIT_SYNC_TIMEOUT_MS);
        if (wait_result == ERROR_TIMEOUT) {
            LOG_W(TAG, "Panel refresh did not finish within %u ms", (unsigned int)LVGL_DISPLAY_WAIT_SYNC_TIMEOUT_MS);
        } else if (wait_result != ERROR_NONE) {
            LOG_W(TAG, "Waiting for panel refresh failed: %d", (int)wait_result);
        }

        if (ctx->partial_refresh) {
            // The panel just finished driving; mirror the last cycle's windows into the base image
            // plane before any new cycle can write to the display RAM, keeping the differential
            // refresh sequence diffling against the frame that is actually on screen.
            error_t commit_result = display_commit_base(ctx->refresh_task_device);
            if (commit_result != ERROR_NONE) {
                LOG_W(TAG, "Committing base image failed: %d", (int)commit_result);
            }
        }

        // One writer per direction, no lock needed: only this task clears refresh_in_flight, only
        // lvgl_display_flush_cb() sets it.
        ctx->refresh_in_flight.store(false);

        if (ctx->frame_pending.load()) {
            // A frame or partial cycle was dropped while the panel was busy (see the drop paths
            // in lvgl_display_flush_cb()). Repaint from LVGL's current state so the panel
            // eventually shows the newest content instead of a stale frame: for FULL mode
            // invalidating any area redraws the whole screen; for partial mode it re-runs the
            // banded cycle that was dropped. Either way the invalidate also resumes LVGL's
            // refresh timer, which is paused between refreshes.
            ctx->frame_pending.store(false);
            lvgl_lock();
            lv_obj_t* screen = lv_display_get_screen_active(ctx->refresh_task_display);
            if (screen != NULL) {
                lv_obj_invalidate(screen);
            }
            lvgl_unlock();
        }
    }
    return 0;
}

static void lvgl_display_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* color_map) {
    struct LvglDeviceContext* wrapper = (struct LvglDeviceContext*)lv_display_get_driver_data(disp);
    struct LvglDisplayCtx* ctx = (struct LvglDisplayCtx*)wrapper->context;
    bool is_i1 = lv_display_get_color_format(disp) == LV_COLOR_FORMAT_I1;

    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2;
    int32_t y2 = area->y2;
    uint32_t area_size_px = (uint32_t)(x2 - x1 + 1) * (uint32_t)(y2 - y1 + 1);

    lv_display_rotation_t rotation = lv_display_get_rotation(disp);
    bool rotating = ctx->sw_rotate && rotation != LV_DISPLAY_ROTATION_0;

    // Windowed partial refresh (I1 e-paper): each flush_cb call is one tile of the
    // current cycle. Tiles stream into the panel RAM as they arrive; the panel only
    // drives once per cycle, triggered from the last flush. Unlike FULL mode there
    // is no complete frame anywhere to present, so every tile must be handled.
    if (ctx->partial_refresh) {
        // First flush of a cycle decides whether the cycle runs or is dropped.
        if (!ctx->cycle_active) {
            ctx->cycle_active = true;
            ctx->cycle_dropped = false;
            if (ctx->refresh_in_flight.load()) {
                // The panel is still driving the previous cycle. Tiles written now would
                // be ignored (GDDRAM is read-only while BUSY) and the base plane is not
                // committed yet, so drop the whole cycle; the refresh task re-renders
                // once the panel is idle (see lvgl_display_refresh_task_main).
                ctx->cycle_dropped = true;
                ctx->frame_pending.store(true);
            } else if (rotating) {
                // Windowed refresh only supports the panel's base orientation; rotated
                // content cannot be placed in the panel RAM correctly.
                ctx->cycle_dropped = true;
                LOG_E(TAG, "Partial refresh does not support rotation, dropping cycle");
            }
        }

        if (!ctx->cycle_dropped) {
            // LVGL reserves an 8-byte palette at the front of every I1 draw buffer; the
            // tile's pixels follow it, tightly packed at the area's width.
            error_t draw_result = display_draw_bitmap(wrapper->device, x1, y1, x2 + 1, y2 + 1, color_map + 8);
            if (draw_result != ERROR_NONE) {
                LOG_W(TAG, "draw_bitmap failed: %d", (int)draw_result);
            }
        }

        if (lv_display_flush_is_last(disp)) {
            // End of the cycle: trigger the panel drive for everything streamed so far,
            // then hand the "wait for it to finish" over to the refresh task.
            if (!ctx->cycle_dropped) {
                error_t refresh_result = display_refresh(wrapper->device, false);
                if (refresh_result != ERROR_NONE) {
                    LOG_W(TAG, "refresh failed: %d", (int)refresh_result);
                } else {
                    ctx->refresh_in_flight.store(true);
                    TaskHandle_t task_handle = thread_get_task_handle(ctx->refresh_task);
                    if (task_handle != NULL) {
                        xTaskNotifyGive(task_handle);
                    } else {
                        LOG_E(TAG, "Refresh task not running while streaming a frame");
                    }
                }
            }
            ctx->cycle_active = false;
            ctx->cycle_dropped = false;
        }
        lv_display_flush_ready(disp);
        return;
    }

    // In FULL mode, a refresh cycle can call this once per still-unjoined invalidated area (see
    // the comment below) before the frame is complete - rotating per-tile here would only ever
    // reflect the last tile written, not the accumulated whole frame. Rotate the whole buffer in
    // one shot instead, right before presenting (see the FULL-mode branch below).
    if (rotating && ctx->render_mode != LV_DISPLAY_RENDER_MODE_FULL) {
        // sw_rotate is only ever requested for displays lacking real HW mirror/swap_xy capability
        // (see lvgl_devices.c), and lvgl_display_add() only binds fb-direct (owns_buffers == false)
        // when that capability is present - so this is always the owns_buffers == true case.
        lv_color_format_t color_format = lv_display_get_color_format(disp);
        int32_t w = x2 - x1 + 1;
        int32_t h = y2 - y1 + 1;

        void* ppa_out = lvgl_display_try_ppa_rotate(ctx, color_map, w, h, rotation, color_format);
        if (ppa_out != NULL) {
            color_map = (uint8_t*)ppa_out;
        } else {
            if (lvgl_display_ensure_rotate_buf(ctx) != ERROR_NONE) {
                LOG_E(TAG, "Failed to allocate rotation buffer, dropping frame");
                lv_display_flush_ready(disp);
                return;
            }
            uint32_t w_stride = lv_draw_buf_width_to_stride(w, color_format);
            uint32_t h_stride = lv_draw_buf_width_to_stride(h, color_format);
            if (rotation == LV_DISPLAY_ROTATION_180) {
                lv_draw_sw_rotate(color_map, ctx->rotate_buf, w, h, w_stride, w_stride, rotation, color_format);
            } else {
                lv_draw_sw_rotate(color_map, ctx->rotate_buf, w, h, w_stride, h_stride, rotation, color_format);
            }
            color_map = (uint8_t*)ctx->rotate_buf;
        }
        lv_area_t rotated_area = { x1, y1, x2, y2 };
        lv_display_rotate_area(disp, &rotated_area);
        x1 = rotated_area.x1;
        y1 = rotated_area.y1;
        x2 = rotated_area.x2;
        y2 = rotated_area.y2;
    }

    if (ctx->byte_swap) {
        lv_draw_sw_rgb565_swap(color_map, area_size_px);
    }

    if (ctx->render_mode == LV_DISPLAY_RENDER_MODE_FULL) {
        // FULL mode always redraws (and flushes) the whole display, but a refresh cycle can still
        // call this flush_cb once per still-unjoined invalidated area (lv_refr.c's
        // refr_invalid_areas()), each writing its own tile into the *same* shared buffer - only
        // the last call actually holds the complete frame. This applies equally whether that
        // buffer is one we own (owns_buffers, e.g. an I1 e-paper/OLED panel) or a real hardware
        // frame buffer (fb-direct, where display_draw_bitmap() - see rgb_display_draw_bitmap() -
        // additionally blocks for a full scan-out period whenever frame_buffer_count > 0).
        // Presenting on every call would send partially-rendered frames, and for fb-direct would
        // also pay that scan-out wait N times per refresh instead of once; defer to the last
        // flush and present the whole buffer in one call, mirroring esp_lvgl_port_disp.c's own
        // lv_disp_flush_is_last() gate for its direct/full render mode.
        if (lv_display_flush_is_last(disp)) {
            // Slow-refresh panels (e-paper) take 1-3s per refresh. If the previous refresh is still
            // in flight, drop this frame instead of sending it into a busy panel: the newest content
            // is re-rendered by the refresh task once the panel is idle (lvgl_display_refresh_task_main).
            // The drop is invisible because the panel is mid-refresh right now - it never shows the
            // dropped frame - and skipping avoids queueing stale refreshes behind the panel.
            if (ctx->slow_refresh && ctx->refresh_in_flight.load()) {
                ctx->frame_pending.store(true);
                lv_display_flush_ready(disp);
                return;
            }
            uint8_t* fb_base;
            if (ctx->owns_buffers) {
                fb_base = (uint8_t*)ctx->buf1;
                if (is_i1) {
                    // LVGL reserves an 8-byte palette (2 x lv_color32_t) at the front of every I1
                    // draw buffer; it's on the caller to skip it before treating the rest as
                    // pixel data.
                    fb_base += 8;
                }
            } else {
                fb_base = (uint8_t*)lvgl_display_fb_base(ctx, color_map);
            }
            uint16_t hres = display_get_resolution_x(wrapper->device);
            uint16_t vres = display_get_resolution_y(wrapper->device);

            if (rotating) {
                // fb_base now holds the whole completed frame, but still in LVGL's *logical*
                // (rotated) w/h - rotate it as one block into rotate_buf, matching the panel's
                // fixed physical w/h, before presenting.
                lv_color_format_t color_format = lv_display_get_color_format(disp);
                bool swapped_wh = rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270;
                int32_t logical_w = swapped_wh ? (int32_t)vres : (int32_t)hres;
                int32_t logical_h = swapped_wh ? (int32_t)hres : (int32_t)vres;

                void* ppa_out = lvgl_display_try_ppa_rotate(ctx, fb_base, logical_w, logical_h, rotation, color_format);
                if (ppa_out != NULL) {
                    fb_base = (uint8_t*)ppa_out;
                } else {
                    if (lvgl_display_ensure_rotate_buf(ctx) != ERROR_NONE) {
                        LOG_E(TAG, "Failed to allocate rotation buffer, dropping frame");
                        lv_display_flush_ready(disp);
                        return;
                    }
                    uint32_t src_stride = lv_draw_buf_width_to_stride(logical_w, color_format);
                    uint32_t dest_stride = lv_draw_buf_width_to_stride(hres, color_format);
                    lv_draw_sw_rotate(fb_base, ctx->rotate_buf, logical_w, logical_h, src_stride, dest_stride, rotation, color_format);
                    fb_base = (uint8_t*)ctx->rotate_buf;
                }
            }

            error_t draw_result = display_draw_bitmap(wrapper->device, 0, 0, hres, vres, fb_base);
            if (draw_result != ERROR_NONE) {
                LOG_W(TAG, "draw_bitmap failed: %d", (int)draw_result);
            } else if (ctx->slow_refresh) {
                // The panel is now refreshing on its own; hand the "wait for it to finish" over to
                // the refresh task so the render thread isn't stalled for the whole refresh.
                ctx->refresh_in_flight.store(true);
                TaskHandle_t task_handle = thread_get_task_handle(ctx->refresh_task);
                if (task_handle != NULL) {
                    xTaskNotifyGive(task_handle);
                } else {
                    LOG_E(TAG, "Refresh task not running while streaming a frame");
                }
            }
        }
    } else if (ctx->owns_buffers) {
        // PARTIAL mode: each flush_cb call is one independent, complete tile into a buffer that
        // gets reused for the next tile, so present it immediately rather than waiting.
        // LVGL's area is inclusive; DisplayApi's draw_bitmap wants an exclusive end.
        display_draw_bitmap(wrapper->device, x1, y1, x2 + 1, y2 + 1, color_map);
    }
    // DisplayApi has no async completion callback, so draw_bitmap is synchronous.
    lv_display_flush_ready(disp);
}

error_t lvgl_display_add(struct Device* device, const struct LvglDisplayConfig* config, lv_display_t** out_display) {
    if (device == NULL || config == NULL || out_display == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    if (device_get_type(device) != &DISPLAY_TYPE) {
        return ERROR_INVALID_ARGUMENT;
    }

    lv_color_format_t lv_color_format;
    enum DisplayColorFormat kernel_color_format = display_get_color_format(device);
    if (!lvgl_display_map_color_format(kernel_color_format, &lv_color_format)) {
        LOG_E(TAG, "Unsupported color format %d (no LVGL equivalent)", (int)kernel_color_format);
        return ERROR_NOT_SUPPORTED;
    }

    uint16_t hres = display_get_resolution_x(device);
    uint16_t vres = display_get_resolution_y(device);
    uint8_t fb_count = display_get_frame_buffer_count(device);
    uint8_t bpp = lv_color_format_get_size(lv_color_format);

    struct LvglDisplayCtx* ctx = new(std::nothrow) LvglDisplayCtx();
    if (ctx == NULL) {
        return ERROR_OUT_OF_MEMORY;
    }
    struct LvglDeviceContext* wrapper = new(std::nothrow) LvglDeviceContext(ctx);
    if (wrapper == NULL) {
        delete ctx;
        return ERROR_OUT_OF_MEMORY;
    }
    wrapper->device = device;

    ctx->byte_swap = config->swap_bytes;
    ctx->sw_rotate = config->sw_rotate;
    ctx->prefer_external_ram = config->prefer_external_ram;
    // Only relevant when sw_rotate is set - lvgl_display_try_ppa_rotate() also checks
    // ctx->ppa_eligible directly, so this is safe to compute unconditionally.
    ctx->ppa_eligible = lvgl_ppa_is_supported() && lvgl_ppa_supports_color_format(lv_color_format);
    if (config->sw_rotate && !ctx->ppa_eligible) {
        LOG_I(TAG, "PPA not available for this display (supported=%d, color_format=%d) - using lv_draw_sw_rotate()",
              (int)lvgl_ppa_is_supported(), (int)lv_color_format);
    }
    ctx->has_swap_xy_cap = display_has_capability(device, DISPLAY_CAPABILITY_CAP_SWAP_XY);
    ctx->has_mirror_cap = display_has_capability(device, DISPLAY_CAPABILITY_CAP_MIRROR);
    ctx->has_set_gap_cap = display_has_capability(device, DISPLAY_CAPABILITY_CAP_SET_GAP);
    // sw_rotate is excluded from fb-direct binding below: it writes rotated pixels into
    // ctx->rotate_buf, which lvgl_display_fb_base() doesn't recognize, so fb-direct must stay
    // off in that case as well.
    bool would_bind_fb_direct = fb_count > 0 && ctx->has_swap_xy_cap && ctx->has_mirror_cap && !ctx->sw_rotate;
    if (fb_count > 0 && !would_bind_fb_direct) {
        // Only re-enable capabilities the driver reported off above if it has a *dynamic*
        // has_capability() (DisplayApi.has_capability non-null, e.g. rgb_display_has_capability()/
        // st7701_has_capability()): that's specifically what a driver implements when a capability's
        // availability is state-dependent - here, off only because binding fb-direct would defeat
        // it (see rgb_display_has_capability()) - so once we're falling back to an owned buffer
        // instead (right below), the concern no longer applies. A driver with no dynamic
        // has_capability() reports fixed, hardware-level capabilities instead: if the bit's off,
        // swap_xy()/mirror() don't exist at all (null function pointers - see the DisplayApi
        // contract), so forcing them back on here would call through a null pointer.
        const struct DisplayApi* api = (const struct DisplayApi*)device_get_driver(device)->api;
        if (api->has_capability != NULL) {
            ctx->has_swap_xy_cap = true;
            ctx->has_mirror_cap = true;
        }
    }
    ctx->base_swap_xy = ctx->has_swap_xy_cap ? display_get_swap_xy(device) : false;
    ctx->base_mirror_x = ctx->has_mirror_cap ? display_get_mirror_x(device) : false;
    ctx->base_mirror_y = ctx->has_mirror_cap ? display_get_mirror_y(device) : false;
    ctx->base_gap_x = ctx->has_set_gap_cap ? display_get_gap_x(device) : 0;
    ctx->base_gap_y = ctx->has_set_gap_cap ? display_get_gap_y(device) : 0;

    lv_display_render_mode_t render_mode;
    size_t buf_size_bytes;

    if (lv_color_format == LV_COLOR_FORMAT_I1) {
        // Windowed partial refresh for I1 e-paper that advertises both PARTIAL_REFRESH and
        // SLOW_REFRESH: render into a small owned buffer (buffer_height rows) and stream each
        // flush tile to the panel as it arrives, triggering the panel drive once per cycle
        // (see lvgl_display_flush_cb()). The +8 reserves LVGL's I1 palette header, which
        // get_max_row() subtracts when computing the tile height.
        bool partial_i1 = display_has_capability(device, DISPLAY_CAPABILITY_PARTIAL_REFRESH) &&
            display_has_capability(device, DISPLAY_CAPABILITY_SLOW_REFRESH);
        if (partial_i1) {
            uint16_t buf_height = config->buffer_height == 0 ? vres : config->buffer_height;
            buf_size_bytes = (size_t)((hres + 7) / 8) * buf_height + 8;
            ctx->buf1 = lvgl_display_alloc_buffer(buf_size_bytes, config->prefer_external_ram);
            if (ctx->buf1 == NULL) {
                delete wrapper;
                return ERROR_OUT_OF_MEMORY;
            }
            ctx->owns_buffers = true;
            render_mode = LV_DISPLAY_RENDER_MODE_PARTIAL;
            ctx->partial_refresh = true;
        } else {
            // I1 packs 8 pixels/byte row-wise and LVGL reserves an 8-byte palette header at the
            // buffer's start (see lvgl_display_flush_cb()). Always redraw the whole frame in one
            // owned buffer instead of computing partial-region byte offsets against that packing.
            buf_size_bytes = (size_t)((hres + 7) / 8) * vres + 8;
            ctx->buf1 = lvgl_display_alloc_buffer(buf_size_bytes, config->prefer_external_ram);
            if (ctx->buf1 == NULL) {
                delete wrapper;
                return ERROR_OUT_OF_MEMORY;
            }
            ctx->owns_buffers = true;
            render_mode = LV_DISPLAY_RENDER_MODE_FULL;
        }
    } else if (would_bind_fb_direct) {
        display_get_frame_buffer(device, 0, &ctx->buf1);
        if (fb_count > 1) {
            display_get_frame_buffer(device, 1, &ctx->buf2);
        }
        ctx->owns_buffers = false;
        render_mode = LV_DISPLAY_RENDER_MODE_FULL;
        buf_size_bytes = (size_t)hres * vres * bpp;
    } else {
        uint16_t buf_height = config->force_full_frame || config->buffer_height == 0
            ? vres : config->buffer_height;
        buf_size_bytes = (size_t)hres * buf_height * bpp;

        ctx->buf1 = lvgl_display_alloc_buffer(buf_size_bytes, config->prefer_external_ram);
        if (ctx->buf1 == NULL) {
            delete wrapper;
            return ERROR_OUT_OF_MEMORY;
        }
        if (config->double_buffer) {
            ctx->buf2 = lvgl_display_alloc_buffer(buf_size_bytes, config->prefer_external_ram);
            if (ctx->buf2 == NULL) {
                lvgl_display_free_buffer(ctx->buf1);
                delete wrapper;
                return ERROR_OUT_OF_MEMORY;
            }
        }
        ctx->owns_buffers = true;
        render_mode = config->force_full_frame ? LV_DISPLAY_RENDER_MODE_FULL : LV_DISPLAY_RENDER_MODE_PARTIAL;
    }

    ctx->buf_size_bytes = buf_size_bytes;

    lv_display_t* disp = lv_display_create(hres, vres);
    if (disp == NULL) {
        if (ctx->owns_buffers) {
            lvgl_display_free_buffer(ctx->buf1);
            lvgl_display_free_buffer(ctx->buf2);
        }
        delete wrapper;
        return ERROR_OUT_OF_MEMORY;
    }

    ctx->render_mode = render_mode;
    lv_display_set_color_format(disp, lv_color_format);
    lv_display_set_buffers(disp, ctx->buf1, ctx->buf2, buf_size_bytes, render_mode);
    lv_display_set_flush_cb(disp, lvgl_display_flush_cb);
    lv_display_set_driver_data(disp, wrapper);
    lv_display_add_event_cb(disp, lvgl_display_rotation_event_cb, LV_EVENT_RESOLUTION_CHANGED, wrapper);

    // Apply once explicitly, independent of whether LV_EVENT_RESOLUTION_CHANGED fires on creation.
    lvgl_display_apply_rotation(wrapper, lv_display_get_rotation(disp));

    ctx->slow_refresh = display_has_capability(device, DISPLAY_CAPABILITY_SLOW_REFRESH);
    ctx->refresh_task_device = device;
    ctx->refresh_task_display = disp;
    if (ctx->slow_refresh) {
        // Slow-refresh panels take 1-3s per refresh; waiting for that in the flush callback would
        // stall the render thread, so the wait happens on this dedicated low-priority task instead.
        ctx->refresh_task = thread_alloc_full(
            "lvgl_refresh",
            LVGL_DISPLAY_REFRESH_TASK_STACK_SIZE,
            lvgl_display_refresh_task_main,
            ctx,
            -1 // no CPU affinity (the kernel's no-affinity sentinel; tskNO_AFFINITY is ESP-IDF-only)
        );
        if (ctx->refresh_task == NULL) {
            lv_display_delete(disp);
            if (ctx->owns_buffers) {
                lvgl_display_free_buffer(ctx->buf1);
                lvgl_display_free_buffer(ctx->buf2);
            }
            delete wrapper;
            return ERROR_OUT_OF_MEMORY;
        }
        thread_set_priority(ctx->refresh_task, THREAD_PRIORITY_LOW);
        if (thread_start(ctx->refresh_task) != ERROR_NONE) {
            thread_free(ctx->refresh_task);
            ctx->refresh_task = NULL;
            lv_display_delete(disp);
            if (ctx->owns_buffers) {
                lvgl_display_free_buffer(ctx->buf1);
                lvgl_display_free_buffer(ctx->buf2);
            }
            delete wrapper;
            return ERROR_UNDEFINED;
        }
    }

    *out_display = disp;
    return ERROR_NONE;
}

void lvgl_display_remove(lv_display_t* display) {
    if (display == NULL) {
        return;
    }

    struct LvglDeviceContext* wrapper = (struct LvglDeviceContext*)lv_display_get_driver_data(display);

    if (wrapper != NULL) {
        struct LvglDisplayCtx* ctx = (struct LvglDisplayCtx*)wrapper->context;
        // Stop the refresh task before deleting the display: the task touches the device
        // (display_wait_sync) and LVGL objects (invalidate), both gone after this point, and a
        // still-running task would hold the panel busy while we tear the display down.
        if (ctx->refresh_task != NULL) {
            ctx->refresh_task_stop.store(true);
            TaskHandle_t task_handle = thread_get_task_handle(ctx->refresh_task);
            if (task_handle != NULL) {
                // Wake the task out of ulTaskNotifyTake() so it observes the stop flag and exits.
                xTaskNotifyGive(task_handle);
            }
            if (thread_join(ctx->refresh_task, LVGL_DISPLAY_REFRESH_TASK_JOIN_TIMEOUT, pdMS_TO_TICKS(50)) != ERROR_NONE) {
                // The task only ever stops itself; if it's still alive the driver's wait_sync()
                // ignored its timeout. Leak rather than free memory a live task may still touch.
                LOG_E(TAG, "Refresh task did not stop in time, leaking display resources");
                ctx->refresh_task = NULL;
                return;
            }
            thread_free(ctx->refresh_task);
            ctx->refresh_task = NULL;
        }
    }

    lv_display_delete(display);

    if (wrapper != NULL) {
        struct LvglDisplayCtx* ctx = (struct LvglDisplayCtx*)wrapper->context;
        if (ctx->owns_buffers) {
            if (ctx->buf1 != NULL) {
                lvgl_display_free_buffer(ctx->buf1);
            }
            if (ctx->buf2 != NULL) {
                lvgl_display_free_buffer(ctx->buf2);
            }
        }
        if (ctx->rotate_buf != NULL) {
            lvgl_display_free_buffer(ctx->rotate_buf);
        }
        if (ctx->ppa_handle != NULL) {
            lvgl_ppa_delete(ctx->ppa_handle);
        }
        delete wrapper;
    }
}
