// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <lvgl.h>

#include <tactility/device.h>
#include <tactility/error.h>

#ifdef __cplusplus
extern "C" {
#endif

struct LvglSoftwareKeyboard {
    lv_obj_t* object;
};

/**
 * @brief Creates an lv_indev_t bound to the given KEYBOARD_TYPE device and registers a read callback
 * that polls the device through its KeyboardApi.
 *
 * @warning Caller must hold the LVGL lock (see lvgl_lock() in lvgl_module.h) — call this from
 * LvglModuleConfig.on_start, or after calling lvgl_lock() explicitly.
 *
 * @param[in] device a device of type KEYBOARD_TYPE
 * @param[in] display the display this indev should be associated with, or NULL to leave it unset
 * @param[out] out_indev the created indev, valid only when ERROR_NONE is returned
 * @retval ERROR_NONE on success
 * @retval ERROR_INVALID_ARGUMENT if device or out_indev is NULL, or device is not of type KEYBOARD_TYPE
 * @retval ERROR_OUT_OF_MEMORY if allocation failed
 */
error_t lvgl_keyboard_add(struct Device* device, lv_display_t* display, lv_indev_t** out_indev);

/**
 * @brief Removes an indev previously created with lvgl_keyboard_add().
 * @warning Caller must hold the LVGL lock.
 */
void lvgl_keyboard_remove(lv_indev_t* indev);

void lvgl_software_keyboard_construct(struct LvglSoftwareKeyboard* keyboard, lv_obj_t* parent);

void lvgl_software_keyboard_destruct(struct LvglSoftwareKeyboard* keyboard);

void lvgl_software_keyboard_show(struct LvglSoftwareKeyboard* keyboard, lv_obj_t* textarea);

void lvgl_software_keyboard_hide(struct LvglSoftwareKeyboard* keyboard);

/**
 * The on-screen keyboard is only shown when both of these conditions are true:
 *  - there is no hardware keyboard
 *  - TT_CONFIG_FORCE_ONSCREEN_KEYBOARD is set to true in tactility_config.h
 * @return if we should show a on-screen keyboard for text input inside our apps
 */
bool lvgl_software_keyboard_is_enabled();

struct LvglSoftwareKeyboard* lvgl_software_keyboard_get_last();

void lvgl_keyboard_add_textarea(struct LvglSoftwareKeyboard* keyboard, lv_obj_t* textarea);

void lvgl_software_keyboard_activate(struct LvglSoftwareKeyboard* keyboard);

void lvgl_software_keyboard_deactivate(struct LvglSoftwareKeyboard* keyboard);

/**
 * @return true if LVGL is configured with a keypad
 */
bool lvgl_hardware_keyboard_is_available();

void lvgl_hardware_keyboard_add_custom(lv_indev_t* device);

void lvgl_hardware_keyboard_remove_custom(lv_indev_t* device);

#ifdef __cplusplus
}
#endif
