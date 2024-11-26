#include "ViewPort.h"

#include "Check.h"
#include "service/gui/ViewPort_i.h"
#include "lvgl/Style.h"

namespace tt::service::gui {

#define TAG "viewport"

ViewPort* view_port_alloc(
    app::App& app,
    ViewPortShowCallback on_show,
    ViewPortHideCallback on_hide
) {
    return new ViewPort(
        app,
        on_show,
        on_hide
    );
}

void view_port_free(ViewPort* view_port) {
    tt_assert(view_port);
    delete view_port;
}

void view_port_show(ViewPort* view_port, lv_obj_t* parent) {
    tt_assert(view_port);
    tt_assert(parent);
    if (view_port->onShow) {
        lvgl::obj_set_style_no_padding(parent);
        view_port->onShow(view_port->app, parent);
    }
}

void view_port_hide(ViewPort* view_port) {
    tt_assert(view_port);
    if (view_port->onHide) {
        view_port->onHide(view_port->app);
    }
}

} // namespace
