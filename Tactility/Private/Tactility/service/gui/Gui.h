#pragma once

#include <Tactility/MessageQueue.h>
#include <Tactility/Mutex.h>
#include <Tactility/PubSub.h>

#include "Tactility/app/AppContext.h"

#include <cstdio>

#include <lvgl.h>

namespace tt::service::gui {

#define GUI_THREAD_FLAG_DRAW (1 << 0)
#define GUI_THREAD_FLAG_INPUT (1 << 1)
#define GUI_THREAD_FLAG_EXIT (1 << 2)
#define GUI_THREAD_FLAG_ALL (GUI_THREAD_FLAG_DRAW | GUI_THREAD_FLAG_INPUT | GUI_THREAD_FLAG_EXIT)

/** Gui structure */
struct Gui {
    // Thread and lock
    Thread* thread = nullptr;
    Mutex mutex = Mutex(Mutex::Type::Recursive);
    PubSub::SubscriptionHandle loader_pubsub_subscription = nullptr;

    // Layers and Canvas
    lv_obj_t* appRootWidget = nullptr;
    lv_obj_t* statusbarWidget = nullptr;

    // App-specific
    std::shared_ptr<app::AppContext> appToRender = nullptr;

    lv_obj_t* _Nullable keyboard = nullptr;
    lv_group_t* keyboardGroup = nullptr;
};

/** Update GUI, request redraw */
void requestDraw();

/** Lock GUI */
void lock();

/** Unlock GUI */
void unlock();

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
void softwareKeyboardShow(lv_obj_t* textarea);

/**
 * Hide the on-screen keyboard.
 * Has no effect when the keyboard is not visible.
 */
void softwareKeyboardHide();

/**
 * The on-screen keyboard is only shown when both of these conditions are true:
 *  - there is no hardware keyboard
 *  - TT_CONFIG_FORCE_ONSCREEN_KEYBOARD is set to true in tactility_config.h
 * @return if we should show a on-screen keyboard for text input inside our apps
 */
bool softwareKeyboardIsEnabled();

/**
 * Glue code for the on-screen keyboard and the hardware keyboard:
 *  - Attach automatic hide/show parameters for the on-screen keyboard.
 *  - Registers the textarea to the default lv_group_t for hardware keyboards.
 * @param[in] textarea
 */
void keyboardAddTextArea(lv_obj_t* textarea);

} // namespace
