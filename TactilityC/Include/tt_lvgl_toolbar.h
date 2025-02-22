#pragma once

#include "tt_app.h"
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Create a toolbar widget that shows the app name as title */
lv_obj_t* tt_lvgl_toolbar_create_for_app(lv_obj_t* parent, AppHandle context);

/** Create a toolbar widget with the provided title*/
lv_obj_t* tt_lvgl_toolbar_create(lv_obj_t* parent, const char* title);

/** Sets the toolbar title */
void toolbar_set_title(lv_obj_t* obj, const char* title);

/** Sets the navigation action of the toolbar (button on the top-left)
 * @param[in] obj the toolbar instance
 * @param[in] icon the icon to set on the button
 * @param[in] callback the callback for the click action of the button
 * @param[in] callbackEventUserData the user data that is attached to the callback event object
 */
void toolbar_set_nav_action(lv_obj_t* obj, const char* icon, lv_event_cb_t callback, void* callbackEventUserData);

/**
 * Create and add an action button to the toolbar (aligned to the right of the toolbar)
 * @param[in] obj the toolbar instance
 * @param[in] icon the icon for the action
 * @param[in] callback the callback for the click action of the button
 * @param[in] callbackEventUserData the user data that is attached to the callback event object
 * @return an instance created by lv_button_create()
 */
lv_obj_t* toolbar_add_button_action(lv_obj_t* obj, const char* icon, lv_event_cb_t callback, void* callbackEventUserData);

/**
 * Create and add a switch to the toolbar actions.
 * @param[in] obj the toolbar instance
 * @return an instance created by lv_switch_create()
 */
lv_obj_t* toolbar_add_switch_action(lv_obj_t* obj);

/**
 * Create and add a spinner to the toolbar actions.
 * @param[in] obj the toolbar instance
 * @return an instance created by Tactility's spinner_create()
 */
lv_obj_t* toolbar_add_spinner_action(lv_obj_t* obj);

#ifdef __cplusplus
}
#endif