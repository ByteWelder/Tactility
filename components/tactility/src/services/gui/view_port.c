#include "view_port.h"

#include "check.h"
#include "services/gui/widgets/widgets.h"
#include "view_port_i.h"

#define TAG "viewport"

ViewPort* view_port_alloc(
    Context* context,
    ViewPortShowCallback on_show,
    ViewPortHideCallback on_hide
) {
    ViewPort* view_port = malloc(sizeof(ViewPort));
    view_port->context = context;
    view_port->on_show = on_show;
    view_port->on_hide = on_hide;
    return view_port;
}

void view_port_free(ViewPort* view_port) {
    furi_assert(view_port);
    free(view_port);
}

void view_port_show(ViewPort* view_port, lv_obj_t* parent) {
    furi_assert(view_port);
    furi_assert(parent);
    if (view_port->on_show) {
        lv_obj_set_style_no_padding(parent);
        view_port->on_show(view_port->context, parent);
    }
}

void view_port_hide(ViewPort* view_port) {
    furi_assert(view_port);
    if (view_port->on_hide) {
        view_port->on_hide(view_port->context);
    }
}
