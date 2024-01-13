#include "gui_i.h"

/*
void gui_input_events_callback(const void* value, void* ctx) {
    tt_assert(value);
    tt_assert(ctx);

    Gui* gui = ctx;

    tt_message_queue_put(gui->input_queue, value, FuriWaitForever);
    tt_thread_flags_set(gui->thread_id, GUI_THREAD_FLAG_INPUT);
}

static void gui_input(Gui* gui, InputEvent* input_event) {
    tt_assert(gui);
    tt_assert(input_event);

    // Check input complementarity
    uint8_t key_bit = (1 << input_event->key);
    if(input_event->type == InputTypeRelease) {
        gui->ongoing_input &= ~key_bit;
    } else if(input_event->type == InputTypePress) {
        gui->ongoing_input |= key_bit;
    } else if(!(gui->ongoing_input & key_bit)) {
        TT_LOG_D(
            TAG,
            "non-complementary input, discarding key: %s type: %s, sequence: %p",
            input_get_key_name(input_event->key),
            input_get_type_name(input_event->type),
            (void*)input_event->sequence);
        return;
    }

    gui_lock(gui);

    do {
        if(gui->direct_draw && !gui->ongoing_input_view_port) {
            break;
        }

        ViewPort* view_port = NULL;

        if(gui->lockdown) {
            view_port = gui_view_port_find_enabled(gui->layers[GuiLayerDesktop]);
        } else {
            view_port = gui_view_port_find_enabled(gui->layers[GuiLayerFullscreen]);
            if(!view_port) view_port = gui_view_port_find_enabled(gui->layers[GuiLayerWindow]);
            if(!view_port) view_port = gui_view_port_find_enabled(gui->layers[GuiLayerDesktop]);
        }

        if(!(gui->ongoing_input & ~key_bit) && input_event->type == InputTypePress) {
            gui->ongoing_input_view_port = view_port;
        }

        if(view_port && view_port == gui->ongoing_input_view_port) {
            view_port_input(view_port, input_event);
        } else if(gui->ongoing_input_view_port && input_event->type == InputTypeRelease) {
            TT_LOG_D(
                TAG,
                "ViewPort changed while key press %p -> %p. Sending key: %s, type: %s, sequence: %p to previous view port",
                gui->ongoing_input_view_port,
                view_port,
                input_get_key_name(input_event->key),
                input_get_type_name(input_event->type),
                (void*)input_event->sequence);
            view_port_input(gui->ongoing_input_view_port, input_event);
        } else {
            TT_LOG_D(
                TAG,
                "ViewPort changed while key press %p -> %p. Discarding key: %s, type: %s, sequence: %p",
                gui->ongoing_input_view_port,
                view_port,
                input_get_key_name(input_event->key),
                input_get_type_name(input_event->type),
                (void*)input_event->sequence);
        }
    } while(false);

    gui_unlock(gui);
}
*/
