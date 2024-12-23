#include "lv_screenshot.h"

#include "save_png.h"
#include "save_bmp.h"

#ifdef __cplusplus
extern "C" {
#endif

static void data_pre_processing(lv_draw_buf_t* snapshot, uint16_t bpp, lv_100ask_screenshot_sv_t screenshot_sv);

bool lv_screenshot_create(lv_obj_t* obj, lv_color_format_t cf, lv_100ask_screenshot_sv_t screenshot_sv, const char* filename) {
    lv_draw_buf_t* snapshot = lv_snapshot_take(obj, cf);

    if (snapshot) {
        data_pre_processing(snapshot, LV_COLOR_DEPTH, screenshot_sv);

        bool success = false;
        if (screenshot_sv == LV_100ASK_SCREENSHOT_SV_PNG) {
            if (LV_COLOR_DEPTH == 16) {
                success = lv_screenshot_save_png_file(snapshot->data, snapshot->header.w, snapshot->header.h, 24, filename);
            } else if (LV_COLOR_DEPTH == 32) {
                success = lv_screenshot_save_png_file(snapshot->data, snapshot->header.w, snapshot->header.h, 32, filename);
            }
        } else if (screenshot_sv == LV_100ASK_SCREENSHOT_SV_BMP) {
            if (LV_COLOR_DEPTH == 16) {
                success = lve_screenshot_save_bmp_file(snapshot->data, snapshot->header.w, snapshot->header.h, 24, filename);
            } else if (LV_COLOR_DEPTH == 32) {
                success = lve_screenshot_save_bmp_file(snapshot->data, snapshot->header.w, snapshot->header.h, 32, filename);
            }
        }

        lv_draw_buf_destroy(snapshot);
        return success;
    }

    return false;
}

static void data_pre_processing(lv_draw_buf_t* snapshot, uint16_t bpp, lv_100ask_screenshot_sv_t screenshot_sv) {
    if (bpp == 16) {
        uint16_t rgb565_data = 0;
        uint32_t count = 0;
        for (int w = 0; w < snapshot->header.w; w++) {
            for (int h = 0; h < snapshot->header.h; h++) {
                rgb565_data = (uint16_t)((*(uint8_t*)(snapshot->data + count + 1) << 8) | *(uint8_t*)(snapshot->data + count));
                if (screenshot_sv == LV_100ASK_SCREENSHOT_SV_PNG) {
                    *(uint8_t*)(snapshot->data + count) = (uint8_t)(((rgb565_data) >> 11) << 3);
                    *(uint8_t*)(snapshot->data + count + 1) = (uint8_t)(((rgb565_data) >> 5) << 2);
                    *(uint8_t*)(snapshot->data + count + 2) = (uint8_t)(((rgb565_data) >> 0) << 3);
                } else if (screenshot_sv == LV_100ASK_SCREENSHOT_SV_BMP) {
                    *(uint8_t*)(snapshot->data + count) = (uint8_t)(((rgb565_data) >> 0) << 3);
                    *(uint8_t*)(snapshot->data + count + 1) = (uint8_t)(((rgb565_data) >> 5) << 2);
                    *(uint8_t*)(snapshot->data + count + 2) = (uint8_t)(((rgb565_data) >> 11) << 3);
                }

                count += 3;
            }
        }
    } else if ((screenshot_sv == LV_100ASK_SCREENSHOT_SV_PNG) && (bpp == 32)) {
        uint8_t tmp_data = 0;
        uint32_t count = 0;
        for (int w = 0; w < snapshot->header.w; w++) {
            for (int h = 0; h < snapshot->header.h; h++) {
                tmp_data = *(snapshot->data + count);
                *(uint8_t*)(snapshot->data + count) = *(snapshot->data + count + 2);
                *(uint8_t*)(snapshot->data + count + 2) = tmp_data;
                count += 4;
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
