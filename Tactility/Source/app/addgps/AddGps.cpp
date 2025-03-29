#include "Tactility/StringUtils.h"
#include "Tactility/app/AppManifest.h"
#include "Tactility/app/alertdialog/AlertDialog.h"
#include "Tactility/hal/gps/GpsDevice.h"
#include "Tactility/hal/uart/Uart.h"
#include "Tactility/lvgl/Style.h"
#include "Tactility/lvgl/Toolbar.h"
#include "Tactility/service/gps/GpsService.h"

#include <cstring>
#include <lvgl.h>

namespace tt::app::addgps {

constexpr const char* TAG = "AddGps";

class AddGpsApp final : public App {

private:

    lv_obj_t* uartDropdown = nullptr;
    lv_obj_t* modelDropdown = nullptr;
    lv_obj_t* baudDropdown = nullptr;

    // Store as string instead of int, so app startup doesn't require parsing all entries.
    // We only need to parse back to int when adding the new GPS entry
    std::array<uint32_t, 6> baudRates = { 9600, 19200, 28800, 38400, 57600, 115200 };
    const char* baudRatesDropdownValues = "9600\n19200\n28800\n38400\n57600\n115200";

    static void onAddGpsCallback(lv_event_t* event) {
        auto* app = (AddGpsApp*)lv_event_get_user_data(event);
        app->onAddGps();
    }

    void onAddGps() {
        auto selected_baud_index = lv_dropdown_get_selected(baudDropdown);

        auto new_configuration = hal::gps::GpsConfiguration {
            .uartName = { 0x00 },
            .baudRate = baudRates[selected_baud_index],
            // Warning: This assumes that the enum is a regularly indexed one that starts at 0
            .model = (hal::gps::GpsModel)lv_dropdown_get_selected(modelDropdown)
        };

        lv_dropdown_get_selected_str(uartDropdown, new_configuration.uartName, sizeof(new_configuration.uartName));
        if (new_configuration.uartName[0] == 0x00) {
            alertdialog::start("Error", "You must select a bus/uart.");
            return;
        }

        TT_LOG_I(TAG, "Saving: uart=%s, model=%lu, baud=%lu", new_configuration.uartName, (uint32_t)new_configuration.model, new_configuration.baudRate);

        auto service = service::gps::findGpsService();
        std::vector<tt::hal::gps::GpsConfiguration> configurations;
        if (service != nullptr) {
            service->getGpsConfigurations(configurations);
            for (auto& stored_configuration: configurations) {
                if (strcmp(stored_configuration.uartName, new_configuration.uartName) == 0) {
                    auto message = std::string("Bus \"{}\" is already in use in another configuration", (const char*)new_configuration.uartName);
                    app::alertdialog::start("Error", message.c_str());
                    return;
                }
            }

            if (!service->addGpsConfiguration(new_configuration)) {
                app::alertdialog::start("Error", "Failed to add configuration");
            } else {
                stop();
            }
        }
    }

public:

    void onShow(AppContext& app, lv_obj_t* parent) final {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

        lvgl::toolbar_create(parent, app);

        auto* main_wrapper = lv_obj_create(parent);
        lv_obj_set_width(main_wrapper, LV_PCT(100));
        lv_obj_set_flex_grow(main_wrapper, 1);
        lv_obj_set_flex_flow(main_wrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(main_wrapper, 0, 0);
        lv_obj_set_style_border_width(main_wrapper, 0, 0);
        lvgl::obj_set_style_bg_invisible(main_wrapper);

        // region Uart

        auto* uart_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_size(uart_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_ver(uart_wrapper, 0, 0);
        lv_obj_set_style_border_width(uart_wrapper, 0, 0);
        lvgl::obj_set_style_bg_invisible(uart_wrapper);

        uartDropdown = lv_dropdown_create(uart_wrapper);

        auto uart_names = hal::uart::getNames();
        uart_names.insert(uart_names.begin(), "");
        auto uart_options = string::join(uart_names, "\n");
        lv_dropdown_set_options(uartDropdown, uart_options.c_str());
        lv_obj_align(uartDropdown, LV_ALIGN_TOP_RIGHT, 0, 0);
        lv_obj_set_width(uartDropdown, LV_PCT(50));

        auto* uart_label = lv_label_create(uart_wrapper);
        lv_obj_align(uart_label, LV_ALIGN_TOP_LEFT, 0, 10);
        lv_label_set_text(uart_label, "Bus");

        // region Model

        auto* model_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_size(model_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_ver(model_wrapper, 0, 0);
        lv_obj_set_style_border_width(model_wrapper, 0, 0);
        lvgl::obj_set_style_bg_invisible(model_wrapper);

        modelDropdown = lv_dropdown_create(model_wrapper);

        auto model_names = hal::gps::getModels();
        auto model_options = string::join(model_names, "\n");
        lv_dropdown_set_options(modelDropdown, model_options.c_str());
        lv_obj_align(modelDropdown, LV_ALIGN_TOP_RIGHT, 0, 0);
        lv_obj_set_width(modelDropdown, LV_PCT(50));

        auto* model_label = lv_label_create(model_wrapper);
        lv_obj_align(model_label, LV_ALIGN_TOP_LEFT, 0, 10);
        lv_label_set_text(model_label, "Model");

        // endregion

        // region Baud
        auto* baud_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_size(baud_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_ver(baud_wrapper, 0, 0);
        lv_obj_set_style_border_width(baud_wrapper, 0, 0);
        lvgl::obj_set_style_bg_invisible(baud_wrapper);

        baudDropdown = lv_dropdown_create(baud_wrapper);
        lv_dropdown_set_options(baudDropdown, baudRatesDropdownValues);
        lv_obj_align(baudDropdown, LV_ALIGN_TOP_RIGHT, 0, 0);
        lv_obj_set_width(baudDropdown, LV_PCT(50));

        auto* baud_rate_label = lv_label_create(baud_wrapper);
        lv_obj_align(baud_rate_label, LV_ALIGN_TOP_LEFT, 0, 10);
        lv_label_set_text(baud_rate_label, "Baud");

        // endregion

        // region Button

        auto* button_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_size(button_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_ver(button_wrapper, 0, 0);
        lv_obj_set_style_border_width(button_wrapper, 0, 0);
        lvgl::obj_set_style_bg_invisible(button_wrapper);

        auto* add_button = lv_button_create(button_wrapper);
        lv_obj_align(add_button, LV_ALIGN_TOP_MID, 0, 0);
        auto* add_label = lv_label_create(add_button);
        lv_label_set_text(add_label, "Add");
        lv_obj_add_event_cb(add_button, onAddGpsCallback, LV_EVENT_SHORT_CLICKED, this);

        // endregion
    }
};

extern const AppManifest manifest = {
    .id = "AddGps",
    .name = "Add GPS",
    .icon = LV_SYMBOL_GPS,
    .type = Type::Hidden,
    .createApp = create<AddGpsApp>
};

void start() {
    app::start(manifest.id);
}

} // namespace
