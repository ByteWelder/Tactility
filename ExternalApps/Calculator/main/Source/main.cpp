#include <tt_app.h>
#include "Calculator.h"

static void onShow(AppHandle appHandle, void* data, lv_obj_t* parent) {
    static_cast<Calculator*>(data)->onShow(appHandle, parent);
}

static void* createApp() {
    return new Calculator();
}

static void destroyApp(void* app) {
    delete static_cast<Calculator*>(app);
}

ExternalAppManifest manifest = {
    .createData = createApp,
    .destroyData = destroyApp,
    .onShow = onShow,
};

extern "C" {

int main(int argc, char* argv[]) {
    tt_app_register(&manifest);
    return 0;
}

}
