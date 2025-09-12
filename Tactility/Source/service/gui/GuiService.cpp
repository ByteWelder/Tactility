#include "Tactility/service/gui/GuiService.h"
#include "Tactility/lvgl/LvglSync.h"
#include "Tactility/lvgl/Statusbar.h"
#include "Tactility/lvgl/Style.h"
#include "Tactility/service/loader/Loader.h"

#include <Tactility/Tactility.h>
#include <Tactility/app/AppInstance.h>
#include <Tactility/service/ServiceRegistration.h>

namespace tt::service::gui {

extern const ServiceManifest manifest;

constexpr auto* TAG = "GuiService";

// region AppManifest

void GuiService::onLoaderEvent(loader::LoaderEvent event) {
    if (event == loader::LoaderEvent::ApplicationShowing) {
        auto app_instance = app::getCurrentAppContext();
        showApp(app_instance);
    } else if (event == loader::LoaderEvent::ApplicationHiding) {
        hideApp();
    }
}

int32_t GuiService::guiMain() {
    while (true) {
        uint32_t flags = Thread::awaitFlags(GUI_THREAD_FLAG_ALL, EventFlag::WaitAny, (uint32_t)portMAX_DELAY);

        // When service not started or starting -> exit
        State service_state = getState(manifest.id);
        if (service_state != State::Started && service_state != State::Starting) {
            break;
        }

        // Process and dispatch draw call
        if (flags & GUI_THREAD_FLAG_DRAW) {
            Thread::clearFlags(GUI_THREAD_FLAG_DRAW);
            auto service = findService();
            if (service != nullptr) {
                service->redraw();
            }
        }

        if (flags & GUI_THREAD_FLAG_EXIT) {
            Thread::clearFlags(GUI_THREAD_FLAG_EXIT);
            break;
        }
    }

    return 0;
}

lv_obj_t* GuiService::createAppViews(lv_obj_t* parent) {
    lv_obj_send_event(statusbarWidget, LV_EVENT_DRAW_MAIN, nullptr);
    lv_obj_t* child_container = lv_obj_create(parent);
    lv_obj_set_style_pad_all(child_container, 0, 0);
    lv_obj_set_width(child_container, LV_PCT(100));
    lv_obj_set_flex_grow(child_container, 1);

    if (softwareKeyboardIsEnabled()) {
        keyboard = lv_keyboard_create(parent);
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    } else {
        keyboard = nullptr;
    }

    return child_container;
}

void GuiService::redraw() {
    // Lock GUI and LVGL
    lock();

    if (lvgl::lock(1000)) {
        lv_obj_clean(appRootWidget);

        if (appToRender != nullptr) {

            // Create a default group which adds all objects automatically,
            // and assign all indevs to it.
            // This enables navigation with limited input, such as encoder wheels.
            lv_group_t* group = lv_group_create();
            auto* indev = lv_indev_get_next(nullptr);
            while (indev) {
                lv_indev_set_group(indev, group);
                indev = lv_indev_get_next(indev);
            }
            lv_group_set_default(group);

            app::Flags flags = std::static_pointer_cast<app::AppInstance>(appToRender)->getFlags();
            if (flags.showStatusbar) {
                lv_obj_remove_flag(statusbarWidget, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(statusbarWidget, LV_OBJ_FLAG_HIDDEN);
            }

            lv_obj_t* container = createAppViews(appRootWidget);
            appToRender->getApp()->onShow(*appToRender, container);
        } else {
            TT_LOG_W(TAG, "nothing to draw");
        }

        // Unlock GUI and LVGL
        lvgl::unlock();
    } else {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "LVGL");
    }

    unlock();
}

bool GuiService::onStart(TT_UNUSED ServiceContext& service) {
    auto* screen_root = lv_screen_active();
    if (screen_root == nullptr) {
        TT_LOG_E(TAG, "No display found");
        return false;
    }

    thread = new Thread(
        "gui",
        4096, // Last known minimum was 2800 for launching desktop
        []() { return guiMain(); }
    );

    loader_pubsub_subscription = loader::getPubsub()->subscribe([this](auto event) {
        onLoaderEvent(event);
    });

    lvgl::lock(portMAX_DELAY);

    keyboardGroup = lv_group_create();
    lvgl::obj_set_style_bg_blacken(screen_root);

    lv_obj_t* vertical_container = lv_obj_create(screen_root);
    lv_obj_set_size(vertical_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(vertical_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(vertical_container, 0, 0);
    lv_obj_set_style_pad_gap(vertical_container, 0, 0);
    lvgl::obj_set_style_bg_blacken(vertical_container);

    statusbarWidget = lvgl::statusbar_create(vertical_container);

    auto* app_container = lv_obj_create(vertical_container);
    lv_obj_set_style_pad_all(app_container, 0, 0);
    lv_obj_set_style_border_width(app_container, 0, 0);
    lvgl::obj_set_style_bg_blacken(app_container);
    lv_obj_set_width(app_container, LV_PCT(100));
    lv_obj_set_flex_grow(app_container, 1);
    lv_obj_set_flex_flow(app_container, LV_FLEX_FLOW_COLUMN);

    appRootWidget = app_container;

    lvgl::unlock();

    isStarted = true;

    thread->setPriority(THREAD_PRIORITY_SERVICE);
    thread->start();

    return true;
}

void GuiService::onStop(TT_UNUSED ServiceContext& service) {
    lock();

    loader::getPubsub()->unsubscribe(loader_pubsub_subscription);

    appToRender = nullptr;
    isStarted = false;

    ThreadId thread_id = thread->getId();
    Thread::setFlags(thread_id, GUI_THREAD_FLAG_EXIT);
    thread->join();
    delete thread;

    unlock();

    tt_check(lvgl::lock(1000 / portTICK_PERIOD_MS));
    lv_group_delete(keyboardGroup);
    lvgl::unlock();
}

void GuiService::requestDraw() {
    ThreadId thread_id = thread->getId();
    Thread::setFlags(thread_id, GUI_THREAD_FLAG_DRAW);
}

void GuiService::showApp(std::shared_ptr<app::AppContext> app) {
    lock();
    if (!isStarted) {
        TT_LOG_W(TAG, "Failed to show app %s: GUI not started", app->getManifest().id.c_str());
    } else {
        // Ensure previous app triggers onHide() logic
        if (appToRender != nullptr) {
            hideApp();
        }
        appToRender = std::move(app);
    }
    unlock();
    requestDraw();
}

void GuiService::hideApp() {
    lock();
    if (!isStarted) {
        TT_LOG_W(TAG, "Failed to hide app: GUI not started");
    } else if (appToRender == nullptr) {
        TT_LOG_W(TAG, "hideApp() called but no app is currently shown");
    } else {
        // We must lock the LVGL port, because the viewport hide callbacks
        // might call LVGL APIs (e.g. to remove the keyboard from the screen root)
        tt_check(lvgl::lock(configTICK_RATE_HZ));
        appToRender->getApp()->onHide(*appToRender);
        lvgl::unlock();
        appToRender = nullptr;
    }
    unlock();
}

std::shared_ptr<GuiService> findService() {
    return std::static_pointer_cast<GuiService>(
        findServiceById(manifest.id)
    );
}

extern const ServiceManifest manifest = {
    .id = "Gui",
    .createService = create<GuiService>
};

// endregion

} // namespace
