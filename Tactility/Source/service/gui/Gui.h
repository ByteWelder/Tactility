#pragma once

#include "app/App.h"
#include "ViewPort.h"

namespace tt::service::gui {

typedef struct Gui Gui;

/**
 * Set the app viewport in the gui state and request the gui to draw it.
 *
 * @param app
 * @param on_show
 * @param on_hide
 */
void show_app(app::App& app, ViewPortShowCallback on_show, ViewPortHideCallback on_hide);

/**
 * Hide the current app's viewport.
 * Does not request a re-draw because after hiding the current app,
 * we always show the previous app, and there is always at least 1 app running.
 */
void hide_app();

/**
 * Show the on-screen keyboard.
 * @param textarea the textarea to focus the input for
 */
void keyboard_show(lv_obj_t* textarea);

/**
 * Hide the on-screen keyboard.
 * Has no effect when the keyboard is not visible.
 */
void keyboard_hide();

/**
 * The on-screen keyboard is only shown when both of these conditions are true:
 *  - there is no hardware keyboard
 *  - TT_CONFIG_FORCE_ONSCREEN_KEYBOARD is set to true in tactility_config.h
 * @return if we should show a on-screen keyboard for text input inside our apps
 */
bool keyboard_is_enabled();

/**
 * Glue code for the on-screen keyboard and the hardware keyboard:
 *  - Attach automatic hide/show parameters for the on-screen keyboard.
 *  - Registers the textarea to the default lv_group_t for hardware keyboards.
 * @param textarea
 */
void keyboard_add_textarea(lv_obj_t* textarea);

} // namespace
