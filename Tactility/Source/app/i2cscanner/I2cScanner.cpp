#include "app/i2cscanner/I2cScannerPrivate.h"
#include "app/i2cscanner/I2cScannerThread.h"
#include "app/i2cscanner/I2cHelpers.h"

#include "Assets.h"
#include "Tactility.h"
#include "app/AppContext.h"
#include "lvgl/LvglSync.h"
#include "lvgl/Toolbar.h"
#include "service/loader/Loader.h"

#define START_SCAN_TEXT "Scan"
#define STOP_SCAN_TEXT "Stop scan"

namespace tt::app::i2cscanner {

extern const AppManifest manifest;

class I2cScannerApp : public App {

private:

    // Core
    Mutex mutex = Mutex(Mutex::Type::Recursive);
    std::unique_ptr<Timer> scanTimer = nullptr;
    // State
    ScanState scanState = ScanStateInitial;
    i2c_port_t port = I2C_NUM_0;
    std::vector<uint8_t> scannedAddresses;
    // Widgets
    lv_obj_t* scanButtonLabelWidget = nullptr;
    lv_obj_t* portDropdownWidget = nullptr;
    lv_obj_t* scanListWidget = nullptr;

    static void onSelectBusCallback(lv_event_t* event);
    static void onPressScanCallback(lv_event_t* event);
    static void onScanTimerCallback(std::shared_ptr<void> context);

    void onSelectBus(lv_event_t* event);
    void onPressScan(lv_event_t* event);
    void onScanTimer();

    bool shouldStopScanTimer();
    bool getPort(i2c_port_t* outPort);
    bool addAddressToList(uint8_t address);
    bool hasScanThread();
    void startScanning();
    void stopScanning();

    void updateViews();
    void updateViewsSafely();

    void onScanTimerFinished();

public:

    void onShow(AppContext& app, lv_obj_t* parent) override;
    void onHide(AppContext& app) override;
};

/** Returns the app data if the app is active. Note that this could clash if the same app is started twice and a background thread is slow. */
std::shared_ptr<I2cScannerApp> _Nullable optApp() {
    auto appContext = service::loader::getCurrentAppContext();
    if (appContext != nullptr && appContext->getManifest().id == manifest.id) {
        return std::static_pointer_cast<I2cScannerApp>(appContext->getApp());
    } else {
        return nullptr;
    }
}

// region Lifecycle


void I2cScannerApp::onShow(AppContext& app, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

    lvgl::toolbar_create(parent, app);

    lv_obj_t* main_wrapper = lv_obj_create(parent);
    lv_obj_set_flex_flow(main_wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(main_wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(main_wrapper, 1);

    lv_obj_t* wrapper = lv_obj_create(main_wrapper);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_height(wrapper, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(wrapper, 0, 0);
    lv_obj_set_style_border_width(wrapper, 0, 0);

    lv_obj_t* scan_button = lv_button_create(wrapper);
    lv_obj_set_width(scan_button, LV_PCT(48));
    lv_obj_align(scan_button, LV_ALIGN_TOP_LEFT, 0, 1); // Shift 1 pixel to align with selection box
    lv_obj_add_event_cb(scan_button, onPressScanCallback, LV_EVENT_SHORT_CLICKED, this);
    lv_obj_t* scan_button_label = lv_label_create(scan_button);
    lv_obj_align(scan_button_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(scan_button_label, START_SCAN_TEXT);
    scanButtonLabelWidget = scan_button_label;

    lv_obj_t* port_dropdown = lv_dropdown_create(wrapper);
    std::string dropdown_items = getPortNamesForDropdown();
    lv_dropdown_set_options(port_dropdown, dropdown_items.c_str());
    lv_obj_set_width(port_dropdown, LV_PCT(48));
    lv_obj_align(port_dropdown, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_add_event_cb(port_dropdown, onSelectBusCallback, LV_EVENT_VALUE_CHANGED, this);
    lv_dropdown_set_selected(port_dropdown, 0);
    portDropdownWidget = port_dropdown;

    lv_obj_t* scan_list = lv_list_create(main_wrapper);
    lv_obj_set_style_margin_top(scan_list, 8, 0);
    lv_obj_set_width(scan_list, LV_PCT(100));
    lv_obj_set_height(scan_list, LV_SIZE_CONTENT);
    lv_obj_add_flag(scan_list, LV_OBJ_FLAG_HIDDEN);
    scanListWidget = scan_list;
}

void I2cScannerApp::onHide(AppContext& app) {
    bool isRunning = false;
    if (mutex.acquire(250 / portTICK_PERIOD_MS) == TtStatusOk) {
        auto* timer = scanTimer.get();
        if (timer != nullptr) {
            isRunning = timer->isRunning();
        }
        mutex.release();
    } else {
        return;
    }

    if (isRunning) {
        stopScanning();
    }
}

// endregion Lifecycle

// region Callbacks

void I2cScannerApp::onSelectBusCallback(lv_event_t* event) {
    auto* app = (I2cScannerApp*)lv_event_get_user_data(event);
    if (app != nullptr) {
        app->onSelectBus(event);
    }
}

void I2cScannerApp::onPressScanCallback(lv_event_t* event) {
    auto* app = (I2cScannerApp*)lv_event_get_user_data(event);
    if (app != nullptr) {
        app->onPressScan(event);
    }
}

void I2cScannerApp::onScanTimerCallback(TT_UNUSED std::shared_ptr<void> context) {
    auto app = optApp();
    if (app != nullptr) {
        app->onScanTimer();
    }
}

// endregion Callbacks

bool I2cScannerApp::getPort(i2c_port_t* outPort) {
    if (mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        *outPort = this->port;
        assert(mutex.release() == TtStatusOk);
        return true;
    } else {
        TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "getPort");
        return false;
    }
}

bool I2cScannerApp::addAddressToList(uint8_t address) {
    if (mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        scannedAddresses.push_back(address);
        assert(mutex.release() == TtStatusOk);
        return true;
    } else {
        TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "addAddressToList");
        return false;
    }
}

bool I2cScannerApp::shouldStopScanTimer() {
    if (mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        bool is_scanning = scanState == ScanStateScanning;
        tt_check(mutex.release() == TtStatusOk);
        return !is_scanning;
    } else {
        return true;
    }
}

void I2cScannerApp::onScanTimer() {
    TT_LOG_I(TAG, "Scan thread started");

    for (uint8_t address = 0; address < 128; ++address) {
        i2c_port_t safe_port;
        if (getPort(&safe_port)) {
            if (hal::i2c::masterHasDeviceAtAddress(port, address, 10 / portTICK_PERIOD_MS)) {
                TT_LOG_I(TAG, "Found device at address %d", address);
                if (!shouldStopScanTimer()) {
                    addAddressToList(address);
                } else {
                    break;
                }
            }
        } else {
            TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "onScanTimer");
            break;
        }

        if (shouldStopScanTimer()) {
            break;
        }
    }

    TT_LOG_I(TAG, "Scan thread finalizing");

    onScanTimerFinished();

    TT_LOG_I(TAG, "Scan timer done");
}

bool I2cScannerApp::hasScanThread() {
    bool has_thread;
    if (mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        has_thread = scanTimer != nullptr;
        tt_check(mutex.release() == TtStatusOk);
        return has_thread;
    } else {
        // Unsafe way
        TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "hasScanTimer");
        return scanTimer != nullptr;
    }
}

void I2cScannerApp::startScanning() {
    if (hasScanThread()) {
        stopScanning();
    }

    if (mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        scannedAddresses.clear();

        lv_obj_add_flag(scanListWidget, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clean(scanListWidget);

        scanState = ScanStateScanning;
        scanTimer = std::make_unique<Timer>(
            Timer::Type::Once,
            onScanTimerCallback
        );
        scanTimer->start(10);
        tt_check(mutex.release() == TtStatusOk);
    } else {
        TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "startScanning");
    }
}
void I2cScannerApp::stopScanning() {
    if (mutex.acquire(250 / portTICK_PERIOD_MS) == TtStatusOk) {
        assert(scanTimer != nullptr);
        scanState = ScanStateStopped;
        tt_check(mutex.release() == TtStatusOk);
    } else {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
    }
}

void I2cScannerApp::onSelectBus(lv_event_t* event) {
    auto* dropdown = static_cast<lv_obj_t*>(lv_event_get_target(event));
    uint32_t selected = lv_dropdown_get_selected(dropdown);
    auto i2c_devices = tt::getConfiguration()->hardware->i2c;
    assert(selected < i2c_devices.size());

    if (mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        scannedAddresses.clear();
        port = i2c_devices[selected].port;
        scanState = ScanStateInitial;
        tt_check(mutex.release() == TtStatusOk);

        updateViews();
    }

    TT_LOG_I(TAG, "Selected %ld", selected);
}

void I2cScannerApp::onPressScan(TT_UNUSED lv_event_t* event) {
    if (scanState == ScanStateScanning) {
        stopScanning();
    } else {
        startScanning();
    }
    updateViews();
}

void I2cScannerApp::updateViews() {
    if (mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        if (scanState == ScanStateScanning) {
            lv_label_set_text(scanButtonLabelWidget, STOP_SCAN_TEXT);
            lv_obj_remove_flag(portDropdownWidget, LV_OBJ_FLAG_CLICKABLE);
        } else {
            lv_label_set_text(scanButtonLabelWidget, START_SCAN_TEXT);
            lv_obj_add_flag(portDropdownWidget, LV_OBJ_FLAG_CLICKABLE);
        }

        lv_obj_clean(scanListWidget);
        if (scanState == ScanStateStopped) {
            lv_obj_remove_flag(scanListWidget, LV_OBJ_FLAG_HIDDEN);
            if (!scannedAddresses.empty()) {
                for (auto address: scannedAddresses) {
                    std::string address_text = getAddressText(address);
                    lv_list_add_text(scanListWidget, address_text.c_str());
                }
            } else {
                lv_list_add_text(scanListWidget, "No devices found");
            }
        } else {
            lv_obj_add_flag(scanListWidget, LV_OBJ_FLAG_HIDDEN);
        }

        tt_check(mutex.release() == TtStatusOk);
    } else {
        TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "updateViews");
    }
}

void I2cScannerApp::updateViewsSafely() {
    if (lvgl::lock(100 / portTICK_PERIOD_MS)) {
        updateViews();
        lvgl::unlock();
    } else {
        TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "updateViewsSafely");
    }
}

void I2cScannerApp::onScanTimerFinished() {
    if (mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        if (scanState == ScanStateScanning) {
            scanState = ScanStateStopped;
            updateViewsSafely();
        }
        tt_check(mutex.release() == TtStatusOk);
    } else {
        TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "onScanTimerFinished");
    }
}

extern const AppManifest manifest = {
    .id = "I2cScanner",
    .name = "I2C Scanner",
    .icon = TT_ASSETS_APP_ICON_I2C_SETTINGS,
    .type = Type::System,
    .createApp = create<I2cScannerApp>
};

void start() {
    service::loader::startApp(manifest.id);
}

} // namespace
