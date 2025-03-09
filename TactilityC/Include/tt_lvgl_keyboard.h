#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Show the on-screen keyboard.
 * @param[in] textarea the textarea to focus the input for
 */
void tt_lvgl_software_keyboard_show(lv_obj_t* textarea);

/**
 * Hide the on-screen keyboard.
 * Has no effect when the keyboard is not visible.
 */
void tt_lvgl_software_keyboard_hide();

/**
 * The on-screen keyboard is only shown when both of these conditions are true:
 *  - there is no hardware keyboard
 *  - TT_CONFIG_FORCE_ONSCREEN_KEYBOARD is set to true in tactility_config.h
 * @return if we should show a on-screen keyboard for text input inside our apps
 */
bool tt_lvgl_software_keyboard_is_enabled();

/**
 * Activate the keypad for a widget group.
 * @param group
 */
void tt_lvgl_software_keyboard_activate(lv_group_t* group);

/**
 * Deactivate the keypad for the current widget group (if any).
 * You don't have to call this after calling _activate() because widget
 * cleanup automatically removes itself from the group it belongs to.
 */
void tt_lvgl_software_keyboard_deactivate();

/**
 * @return true if LVGL is configured with a keypad
 */
bool tt_lvgl_hardware_keyboard_is_available();

/**
 * Set the keypad.
 * @param device the keypad device
 */
void tt_lvgl_hardware_keyboard_set_indev(lv_indev_t* device);

/**
 * Glue code for the on-screen keyboard and the hardware keyboard:
 *  - Attach automatic hide/show parameters for the on-screen keyboard.
 *  - Registers the textarea to the default lv_group_t for hardware keyboards.
 * @param[in] textarea
 */
void tt_lvgl_keyboard_add_textarea(lv_obj_t* textarea);

#ifdef __cplusplus
}
#endif