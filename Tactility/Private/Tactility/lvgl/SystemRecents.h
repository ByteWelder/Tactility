#pragma once

namespace tt::lvgl {

/**
 * Shows the recent-apps switcher: the app stack as horizontal cards, most recent first.
 * Tapping a card raises that app's window (it keeps running), swiping a card up closes that app.
 * @warning Caller must hold the LVGL lock.
 */
void systemRecentsShow();

}
