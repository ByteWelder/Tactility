#pragma once

#include "gui.h"
#include "message_queue.h"
#include "mutex.h"
#include "pubsub.h"
#include "view_port.h"
#include "view_port_i.h"
#include <stdio.h>

#define GUI_THREAD_FLAG_DRAW (1 << 0)
#define GUI_THREAD_FLAG_INPUT (1 << 1)
#define GUI_THREAD_FLAG_EXIT (1 << 2)
#define GUI_THREAD_FLAG_ALL (GUI_THREAD_FLAG_DRAW | GUI_THREAD_FLAG_INPUT | GUI_THREAD_FLAG_EXIT)

/** Gui structure */
struct Gui {
    // Thread and lock
    Thread* thread;
    Mutex mutex;

    // Layers and Canvas
    lv_obj_t* lvgl_parent;

    // App-specific
    ViewPort* app_view_port;

    lv_obj_t* _Nullable toolbar;
    lv_obj_t* _Nullable keyboard;
};

/** Update GUI, request redraw
 */
void gui_request_draw();

/** Lock GUI
 */
void gui_lock();

/** Unlock GUI
 */
void gui_unlock();
