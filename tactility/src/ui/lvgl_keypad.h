/**
 * This code relates to the hardware keyboard support also known as "keypads" in LVGL.
 */
#pragma once

#include "lvgl.h"

/**
 * @return true if LVGL is configured with a keypad
 */
bool tt_lvgl_keypad_is_available();

/**
 * Set the keypad.
 * @param device the keypad device
 */
void tt_lvgl_keypad_set_indev(lv_indev_t* device);

/**
 * Activate the keypad for a widget group.
 * @param group
 */
void tt_lvgl_keypad_activate(lv_group_t* group);

/**
 * Deactivate the keypad for the current widget group (if any).
 * You don't have to call this after calling _activate() because widget
 * cleanup automatically removes itself from the group it belongs to.
 */
void tt_lvgl_keypad_deactivate();