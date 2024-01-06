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
    FuriThread* thread;
    FuriMutex* mutex;

    // Layers and Canvas
    ViewPort* layers[GuiLayerMAX];
    lv_obj_t* lvgl_parent;

    GuiLayer active_layer;

    // Input
    /*
    FuriMessageQueue* input_queue;
    FuriPubSub* input_events;
    uint8_t ongoing_input;
    ViewPort* ongoing_input_view_port;
    */
};

/** Update GUI, request redraw
 */
void gui_request_draw();

///** Input event callback
// *
// * Used to receive input from input service or to inject new input events
// *
// * @param[in]  value  The value pointer (InputEvent*)
// * @param      ctx    The context (Gui instance)
// */
//void gui_input_events_callback(const void* value, void* ctx);

/** Lock GUI
 */
void gui_lock();

/** Unlock GUI
 */
void gui_unlock();
