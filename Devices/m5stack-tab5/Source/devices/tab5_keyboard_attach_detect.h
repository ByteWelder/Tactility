#pragma once

// Starts/stops the periodic keyboard-accessory hot-plug poll: reapplies register configuration on
// reattach (see tab5_keyboard_reinit()) and switches LVGL to landscape while attached, restoring
// the prior rotation on detach - unless the user changed it manually since attaching, in which
// case their choice is respected. Also re-announces the current attach state after LVGL itself
// restarts (e.g. an app that took over the display for direct rendering, stopping and letting
// LVGL rebind), since that's a distinct event from the keyboard physically attaching/detaching -
// the accessory may never have moved, but the rotation override it applied may have been reset in
// the meantime. Called from module.cpp's start()/stop().
void tab5_keyboard_attach_detect_start();
void tab5_keyboard_attach_detect_stop();
