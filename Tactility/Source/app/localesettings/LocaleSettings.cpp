#include <Tactility/Assets.h>
#include <Tactility/app/timezone/TimeZone.h>
#include <Tactility/app/localesettings/TextResources.h>
#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/service/loader/Loader.h>
#include <Tactility/settings/Time.h>
#include <Tactility/StringUtils.h>
#include <Tactility/settings/Language.h>

#include <lvgl.h>
#include <map>
#include <sstream>

namespace tt::app::localesettings {

constexpr auto* TAG = "LocaleSettings";

extern const AppManifest manifest;

class LocaleSettingsApp final : public App {
    tt::i18n::TextResources textResources = tt::i18n::TextResources("/system/app/LocaleSettings/i18n");
    Mutex mutex = Mutex(Mutex::Type::Recursive);
    lv_obj_t* timeZoneLabel = nullptr;
    lv_obj_t* regionLabel = nullptr;
    lv_obj_t* languageDropdown = nullptr;
    lv_obj_t* languageLabel = nullptr;

    static void onConfigureTimeZonePressed(TT_UNUSED lv_event_t* event) {
        timezone::start();
    }

    std::map<settings::Language, std::string> languageMap;

    std::string getLanguageOptions() const {
        std::vector<std::string> items;
        for (int i = 0; i < static_cast<int>(settings::Language::count); i++) {
            switch (static_cast<settings::Language>(i)) {
                case settings::Language::en_GB:
                    items.push_back(textResources[i18n::Text::EN_GB]);
                    break;
                case settings::Language::en_US:
                    items.push_back(textResources[i18n::Text::EN_US]);
                    break;
                case settings::Language::fr_FR:
                    items.push_back(textResources[i18n::Text::FR_FR]);
                    break;
                case settings::Language::nl_BE:
                    items.push_back(textResources[i18n::Text::NL_BE]);
                    break;
                case settings::Language::nl_NL:
                    items.push_back(textResources[i18n::Text::NL_NL]);
                    break;
                case settings::Language::count:
                    break;
            }
        }
        return string::join(items, "\n");
    }

    void updateViews() {
        textResources.load();

        lv_label_set_text(regionLabel , textResources[i18n::Text::REGION].c_str());
        lv_label_set_text(languageLabel, textResources[i18n::Text::LANGUAGE].c_str());

        std::string language_options = getLanguageOptions();
        lv_dropdown_set_options(languageDropdown, language_options.c_str());
        lv_dropdown_set_selected(languageDropdown, static_cast<uint32_t>(settings::getLanguage()));
    }

    static void onLanguageSet(lv_event_t* event) {
        auto* dropdown = static_cast<lv_obj_t*>(lv_event_get_target(event));
        auto index = lv_dropdown_get_selected(dropdown);
        auto language = static_cast<settings::Language>(index);
        settings::setLanguage(language);

        auto* self = static_cast<LocaleSettingsApp*>(lv_event_get_user_data(event));
        self->updateViews();
    }

public:

    void onShow(AppContext& app, lv_obj_t* parent) override {
        auto ui_scale = hal::getConfiguration()->uiScale;

        textResources.load();

        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);

        lvgl::toolbar_create(parent, app);

        auto* main_wrapper = lv_obj_create(parent);
        lv_obj_set_flex_flow(main_wrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_width(main_wrapper, LV_PCT(100));
        lv_obj_set_flex_grow(main_wrapper, 1);

        // Region

        auto* region_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_width(region_wrapper, LV_PCT(100));
        lv_obj_set_height(region_wrapper, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(region_wrapper, 0, 0);
        lv_obj_set_style_border_width(region_wrapper, 0, 0);

        regionLabel = lv_label_create(region_wrapper);
        lv_label_set_text(regionLabel , textResources[i18n::Text::REGION].c_str());
        lv_obj_align(regionLabel , LV_ALIGN_LEFT_MID, 0, 0);

        auto* region_button = lv_button_create(region_wrapper);
        lv_obj_align(region_button, LV_ALIGN_RIGHT_MID, 0, 0);
        auto* region_button_image = lv_image_create(region_button);
        lv_obj_add_event_cb(region_button, onConfigureTimeZonePressed, LV_EVENT_SHORT_CLICKED, nullptr);
        lv_image_set_src(region_button_image, LV_SYMBOL_SETTINGS);

        timeZoneLabel = lv_label_create(region_wrapper);
        std::string timeZoneName = settings::getTimeZoneName();
        if (timeZoneName.empty()) {
            timeZoneName = "not set";
        }

        lv_label_set_text(timeZoneLabel, timeZoneName.c_str());
        const int offset = ui_scale == hal::UiScale::Smallest ? -2 : -10;
        lv_obj_align_to(timeZoneLabel, region_button, LV_ALIGN_OUT_LEFT_MID, offset, 0);

        // Language

        auto* language_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_width(language_wrapper, LV_PCT(100));
        lv_obj_set_height(language_wrapper, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(language_wrapper, 0, 0);
        lv_obj_set_style_border_width(language_wrapper, 0, 0);

        languageLabel = lv_label_create(language_wrapper);
        lv_label_set_text(languageLabel, textResources[i18n::Text::LANGUAGE].c_str());
        lv_obj_align(languageLabel, LV_ALIGN_LEFT_MID, 0, 0);

        languageDropdown = lv_dropdown_create(language_wrapper);
        lv_obj_align(languageDropdown, LV_ALIGN_RIGHT_MID, 0, 0);
        std::string language_options = getLanguageOptions();
        lv_dropdown_set_options(languageDropdown, language_options.c_str());
        lv_dropdown_set_selected(languageDropdown, static_cast<uint32_t>(settings::getLanguage()));
        lv_obj_add_event_cb(languageDropdown, onLanguageSet, LV_EVENT_VALUE_CHANGED, this);
    }

    void onResult(AppContext& app, TT_UNUSED LaunchId launchId, Result result, std::unique_ptr<Bundle> bundle) override {
        if (result == Result::Ok && bundle != nullptr) {
            const auto name = timezone::getResultName(*bundle);
            const auto code = timezone::getResultCode(*bundle);
            TT_LOG_I(TAG, "Result name=%s code=%s", name.c_str(), code.c_str());
            settings::setTimeZone(name, code);

            if (!name.empty()) {
                if (lvgl::lock(100 / portTICK_PERIOD_MS)) {
                    lv_label_set_text(timeZoneLabel, name.c_str());
                    lvgl::unlock();
                }
            }
        }
    }
};

extern const AppManifest manifest = {
    .appId = "LocaleSettings",
    .appName = "Region & Language",
    .appIcon = TT_ASSETS_APP_ICON_TIME_DATE_SETTINGS,
    .appCategory = Category::Settings,
    .createApp = create<LocaleSettingsApp>
};

LaunchId start() {
    return app::start(manifest.appId);
}

} // namespace
