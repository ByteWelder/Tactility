#include "ViewPort.h"

#include "Check.h"
#include "service/gui/ViewPort_i.h"
#include "ui/Style.h"

namespace tt::service::gui {

#define TAG "viewport"

ViewPort* view_port_alloc(
    app::App app,
    ViewPortShowCallback on_show,
    ViewPortHideCallback on_hide
) {
    auto* view_port = static_cast<ViewPort*>(malloc(sizeof(ViewPort)));
    view_port->app = app;
    view_port->on_show = on_show;
    view_port->on_hide = on_hide;
    return view_port;
}

void view_port_free(ViewPort* view_port) {
    tt_assert(view_port);
    free(view_port);
}

void view_port_show(ViewPort* view_port, lv_obj_t* parent) {
    tt_assert(view_port);
    tt_assert(parent);
    if (view_port->on_show) {
        lvgl::obj_set_style_no_padding(parent);
        view_port->on_show(view_port->app, parent);
    }
}

void view_port_hide(ViewPort* view_port) {
    tt_assert(view_port);
    if (view_port->on_hide) {
        view_port->on_hide(view_port->app);
    }
}

} // namespace
