#pragma once

#include "app/AppInstance.h"
#include "app/AppContext.h"

namespace tt::service::gui {

typedef struct Gui Gui;

/**
 * Set the app viewport in the gui state and request the gui to draw it.
 * @param[in] app
 */
void showApp(std::shared_ptr<app::AppContext> app);

/**
 * Hide the current app's viewport.
 * Does not request a re-draw because after hiding the current app,
 * we always show the previous app, and there is always at least 1 app running.
 */
void hideApp();

/**
 * Show the on-screen keyboard.
 * @param[in] textarea the textarea to focus the input for
 */
void keyboardShow(lv_obj_t* textarea);

/**
 * Hide the on-screen keyboard.
 * Has no effect when the keyboard is not visible.
 */
void keyboardHide();

/**
 * The on-screen keyboard is only shown when both of these conditions are true:
 *  - there is no hardware keyboard
 *  - TT_CONFIG_FORCE_ONSCREEN_KEYBOARD is set to true in tactility_config.h
 * @return if we should show a on-screen keyboard for text input inside our apps
 */
bool keyboardIsEnabled();

/**
 * Glue code for the on-screen keyboard and the hardware keyboard:
 *  - Attach automatic hide/show parameters for the on-screen keyboard.
 *  - Registers the textarea to the default lv_group_t for hardware keyboards.
 * @param[in] textarea
 */
void keyboardAddTextArea(lv_obj_t* textarea);

} // namespace
