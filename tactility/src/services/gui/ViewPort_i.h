#pragma once

#include "ViewPort.h"

namespace tt::service::gui {

/** Process draw call. Calls on_show callback.
 * To be used by GUI, called on redraw.
 *
 * @param      view_port  ViewPort instance
 * @param      canvas     canvas to draw at
 */
void view_port_show(ViewPort* view_port, lv_obj_t* parent);

/**
 * Process draw clearing call. Calls on_hdie callback.
 * To be used by GUI, called on redraw.
 *
 * @param view_port
 */
void view_port_hide(ViewPort* view_port);

} // namespace
