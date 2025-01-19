#include "ViewPort.h"

#include "Check.h"
#include "app/AppInstance.h"
#include "lvgl/Style.h"
#include "service/gui/ViewPort_i.h"

namespace tt::service::gui {

#define TAG "viewport"

ViewPort* view_port_alloc(app::AppInstance& app) {
    return new ViewPort(app);
}

void view_port_free(ViewPort* view_port) {
    tt_assert(view_port != nullptr);
    delete view_port;
}

void view_port_show(ViewPort* view_port, lv_obj_t* parent) {
    tt_assert(view_port != nullptr);
    tt_assert(parent != nullptr);
    lvgl::obj_set_style_no_padding(parent);
    view_port->app.getApp()->onShow(view_port->app, parent);
}

void view_port_hide(ViewPort* view_port) {
    tt_assert(view_port != nullptr);
    view_port->app.getApp()->onHide(view_port->app);
}

} // namespace
