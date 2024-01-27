#pragma once

#include "app.h"
#include "service_manifest.h"
#include "view_port.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Gui Gui;

/**
 * Set the app viewport in the gui state and request the gui to draw it.
 *
 * @param app
 * @param on_show
 * @param on_hide
 */
void gui_show_app(App app, ViewPortShowCallback on_show, ViewPortHideCallback on_hide);

/**
 * Hide the current app's viewport.
 * Does not request a re-draw because after hiding the current app,
 * we always show the previous app, and there is always at least 1 app running.
 */
void gui_hide_app();

/**
 * Show the on-screen keyboard.
 * @param textarea the textarea to focus the input for
 */
void gui_keyboard_show(lv_obj_t* textarea);

/**
 * Attach automatic hide/show parameters for the keyboard.
 * Also registers the textarea to the default lv_group_t for hardware keyboards.
 * @param textarea
 */
void gui_keyboard_add_textarea(lv_obj_t* textarea);

/**
 * Hide the on-screen keyboard.
 * Has no effect when the keyboard is not visible.
 */
void gui_keyboard_hide();

/**
 * This function is to facilitate hardware keyboards like the one on Lilygo T-Deck.
 * The software keyboard is only shown when both of these conditions are true:
 *  - there is no hardware keyboard
 *  - TT_CONFIG_FORCE_ONSCREEN_KEYBOARD is set to true in tactility_config.h
 * @return if we should show a keyboard for text input inside our apps
 */
bool gui_keyboard_is_enabled();

#ifdef __cplusplus
}
#endif
