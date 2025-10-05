#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Affects LVGL widget style */
enum UiScale {
    /** Ideal for very small non-touch screen devices (e.g. Waveshare S3 LCD 1.3") */
    UiScaleSmallest,
    /** Nothing was changed in the LVGL UI/UX */
    UiScaleDefault
};

/** @return the UI scaling setting for this device. */
UiScale tt_hal_configuration_get_ui_scale();

#ifdef __cplusplus
}
#endif
