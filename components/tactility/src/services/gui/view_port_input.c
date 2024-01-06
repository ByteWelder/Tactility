#include "view_port_input.h"
/*
_Static_assert(InputKeyMAX == 6, "Incorrect InputKey count");
_Static_assert(
    (InputKeyUp == 0 && InputKeyDown == 1 && InputKeyRight == 2 && InputKeyLeft == 3 &&
     InputKeyOk == 4 && InputKeyBack == 5),
    "Incorrect InputKey order");
*/
/** InputKey directional keys mappings for different screen orientations
 *
 */
/*
static const InputKey view_port_input_mapping[ViewPortOrientationMAX][InputKeyMAX] = {
    {InputKeyUp,
        InputKeyDown,
        InputKeyRight,
        InputKeyLeft,
        InputKeyOk,
        InputKeyBack}, //ViewPortOrientationHorizontal
    {InputKeyDown,
        InputKeyUp,
        InputKeyLeft,
        InputKeyRight,
        InputKeyOk,
        InputKeyBack}, //ViewPortOrientationHorizontalFlip
    {InputKeyRight,
        InputKeyLeft,
        InputKeyDown,
        InputKeyUp,
        InputKeyOk,
        InputKeyBack}, //ViewPortOrientationVertical
    {InputKeyLeft,
        InputKeyRight,
        InputKeyUp,
        InputKeyDown,
        InputKeyOk,
        InputKeyBack}, //ViewPortOrientationVerticalFlip
};

static const InputKey view_port_left_hand_input_mapping[InputKeyMAX] =
    {InputKeyDown, InputKeyUp, InputKeyLeft, InputKeyRight, InputKeyOk, InputKeyBack};

static const CanvasOrientation view_port_orientation_mapping[ViewPortOrientationMAX] = {
    [ViewPortOrientationHorizontal] = CanvasOrientationHorizontal,
    [ViewPortOrientationHorizontalFlip] = CanvasOrientationHorizontalFlip,
    [ViewPortOrientationVertical] = CanvasOrientationVertical,
    [ViewPortOrientationVerticalFlip] = CanvasOrientationVerticalFlip,
};

//// Remaps directional pad buttons on Flipper based on ViewPort orientation
static void view_port_map_input(InputEvent* event, ViewPortOrientation orientation) {
    furi_assert(orientation < ViewPortOrientationMAX && event->key < InputKeyMAX);

    if(event->sequence_source != INPUT_SEQUENCE_SOURCE_HARDWARE) {
        return;
    }

    if(orientation == ViewPortOrientationHorizontal ||
       orientation == ViewPortOrientationHorizontalFlip) {
        if(furi_hal_rtc_is_flag_set(FuriHalRtcFlagHandOrient)) {
            event->key = view_port_left_hand_input_mapping[event->key];
        }
    }
    event->key = view_port_input_mapping[orientation][event->key];
}

void view_port_input_callback_set(
    ViewPort* view_port,
    ViewPortInputCallback callback,
    void* context) {
    furi_assert(view_port);
    furi_check(furi_mutex_acquire(view_port->mutex, FuriWaitForever) == FuriStatusOk);
    view_port->input_callback = callback;
    view_port->input_callback_context = context;
    furi_check(furi_mutex_release(view_port->mutex) == FuriStatusOk);
}

void view_port_input(ViewPort* view_port, InputEvent* event) {
    furi_assert(view_port);
    furi_assert(event);
    furi_check(furi_mutex_acquire(view_port->mutex, FuriWaitForever) == FuriStatusOk);
    furi_check(view_port->gui);

    if(view_port->input_callback) {
        ViewPortOrientation orientation = view_port_get_orientation(view_port);
        view_port_map_input(event, orientation);
        view_port->input_callback(event, view_port->input_callback_context);
    }
    furi_check(furi_mutex_release(view_port->mutex) == FuriStatusOk);
}
*/
