#include <Tactility/TactilityConfig.h>
#include <Tactility/lvgl/Toolbar.h>

#include <Tactility/Assets.h>
#include <Tactility/hal/Device.h>
#include <Tactility/Tactility.h>

#include <format>
#include <lvgl.h>
#include <utility>

#ifdef ESP_PLATFORM
#include <esp_vfs_fat.h>
#include <Tactility/MountPoints.h>
#endif

namespace tt::app::systeminfo {

constexpr auto* TAG = "SystemInfo";

static size_t getHeapFree() {
#ifdef ESP_PLATFORM
    return heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
#else
    return 4096 * 1024;
#endif
}

static size_t getHeapTotal() {
#ifdef ESP_PLATFORM
    return heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
#else
    return 8192 * 1024;
#endif
}

static size_t getSpiFree() {
#ifdef ESP_PLATFORM
    return heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
#else
    return 4096 * 1024;
#endif
}

static size_t getSpiTotal() {
#ifdef ESP_PLATFORM
    return heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
#else
    return 8192 * 1024;
#endif
}

enum class StorageUnit {
    Bytes,
    Kilobytes,
    Megabytes,
    Gigabytes
};

static StorageUnit getStorageUnit(uint64_t value) {
    using enum StorageUnit;
    if (value / (1024 * 1024 * 1024) > 0) {
        return Gigabytes;
    } else if (value / (1024 * 1024) > 0) {
        return Megabytes;
    } else if (value / 1024 > 0) {
        return Kilobytes;
    } else {
        return Bytes;
    }
}

static std::string getStorageUnitString(StorageUnit unit) {
    using enum StorageUnit;
    switch (unit) {
        case Bytes:
            return "bytes";
        case Kilobytes:
            return "kB";
        case Megabytes:
            return "MB";
        case Gigabytes:
            return "GB";
        default:
            std::unreachable();
    }
}

static std::string getStorageValue(StorageUnit unit, uint64_t bytes) {
    using enum StorageUnit;
    switch (unit) {
        case Bytes:
            return std::to_string(bytes);
        case Kilobytes:
            return std::to_string(bytes / 1024);
        case Megabytes:
            return std::format("{:.1f}", static_cast<float>(bytes) / 1024.f / 1024.f);
        case Gigabytes:
            return std::format("{:.1f}", static_cast<float>(bytes) / 1024.f / 1024.f / 1024.f);
        default:
            std::unreachable();
    }
}

static void addMemoryBar(lv_obj_t* parent, const char* label, uint64_t free, uint64_t total) {
    uint64_t used = total - free;
    auto* container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(container, 0, LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_opa(container, 0, LV_STATE_DEFAULT);

    auto* left_label = lv_label_create(container);
    lv_label_set_text(left_label, label);
    lv_obj_set_width(left_label, 60);

    auto* bar = lv_bar_create(container);
    lv_obj_set_flex_grow(bar, 1);

    // Scale down the uint64_t until it fits int32_t for the lv_bar
    uint64_t free_scaled = free;
    uint64_t total_scaled = total;
    while (total_scaled > static_cast<uint64_t>(INT32_MAX)) {
        free_scaled /= 1024;
        total_scaled /= 1024;
    }

    if (total > 0) {
        lv_bar_set_range(bar, 0, total_scaled);
    } else {
        lv_bar_set_range(bar, 0, 1);
    }

    lv_bar_set_value(bar, (total_scaled - free_scaled), LV_ANIM_OFF);

    auto* bottom_label = lv_label_create(parent);
    const auto unit = getStorageUnit(total);
    const auto unit_label = getStorageUnitString(unit);
    const auto used_converted = getStorageValue(unit, used);
    const auto total_converted = getStorageValue(unit, total);
    lv_label_set_text_fmt(bottom_label, "%s / %s %s used", used_converted.c_str(), total_converted.c_str(), unit_label.c_str());
    lv_obj_set_width(bottom_label, LV_PCT(100));
    lv_obj_set_style_text_align(bottom_label, LV_TEXT_ALIGN_RIGHT, 0);

    if (hal::getConfiguration()->uiScale == hal::UiScale::Smallest) {
        lv_obj_set_style_pad_bottom(bottom_label, 2, LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_pad_bottom(bottom_label, 12, LV_STATE_DEFAULT);
    }
}

#if configUSE_TRACE_FACILITY

static const char* getTaskState(const TaskStatus_t& task) {
    switch (task.eCurrentState) {
        case eRunning:
            return "running";
        case eReady:
            return "ready";
        case eBlocked:
            return "blocked";
        case eSuspended:
            return "suspended";
        case eDeleted:
            return "deleted";
        case eInvalid:
        default:
            return "invalid";
    }
}

static void addRtosTask(lv_obj_t* parent, const TaskStatus_t& task) {
    auto* label = lv_label_create(parent);
    const char* name = (task.pcTaskName == nullptr || task.pcTaskName[0] == 0) ? "(unnamed)" : task.pcTaskName;
    lv_label_set_text_fmt(label, "%s (%s)", name, getTaskState(task));
}

static void addRtosTasks(lv_obj_t* parent) {
    UBaseType_t count = uxTaskGetNumberOfTasks();
    auto* tasks = (TaskStatus_t*)malloc(sizeof(TaskStatus_t) * count);
    uint32_t totalRuntime = 0;
    UBaseType_t actual = uxTaskGetSystemState(tasks, count, &totalRuntime);

    for (int i = 0; i < actual; ++i) {
        const TaskStatus_t& task = tasks[i];
        addRtosTask(parent, task);
    }
    free(tasks);
}

#endif

static void addDevice(lv_obj_t* parent, const std::shared_ptr<hal::Device>& device) {
    auto* label = lv_label_create(parent);
    lv_label_set_text(label, device->getName().c_str());
}

static void addDevices(lv_obj_t* parent) {
    auto devices = hal::getDevices();
    for (const auto& device: devices) {
        addDevice(parent, device);
    }
}

static lv_obj_t* createTab(lv_obj_t* tabview, const char* name) {
    auto* tab = lv_tabview_add_tab(tabview, name);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(tab, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(tab, 0, LV_STATE_DEFAULT);
    return tab;
}

class SystemInfoApp final : public App {

    void onShow(AppContext& app, lv_obj_t* parent) override {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);
        lvgl::toolbar_create(parent, app);

        // This wrapper automatically has its children added vertically underneath eachother
        auto* wrapper = lv_obj_create(parent);
        lv_obj_set_style_border_width(wrapper, 0, LV_STATE_DEFAULT);
        lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_width(wrapper, LV_PCT(100));
        lv_obj_set_flex_grow(wrapper, 1);
        lv_obj_set_style_pad_all(wrapper, 0, LV_STATE_DEFAULT);

        auto* tabview = lv_tabview_create(wrapper);
        lv_tabview_set_tab_bar_position(tabview, LV_DIR_LEFT);
        lv_tabview_set_tab_bar_size(tabview, 80);

        // Tabs

        auto* memory_tab = createTab(tabview, "Memory");
        auto* storage_tab = createTab(tabview, "Storage");
        auto* tasks_tab = createTab(tabview, "Tasks");
        auto* devices_tab = createTab(tabview, "Devices");
        auto* about_tab = createTab(tabview, "About");

        // Memory tab content

        addMemoryBar(memory_tab, "Internal", getHeapFree(), getHeapTotal());
        if (getSpiTotal() > 0) {
            addMemoryBar(memory_tab, "External", getSpiFree(), getSpiTotal());
        }

#ifdef ESP_PLATFORM
        // Wrapper for the memory usage bars
        uint64_t storage_total = 0;
        uint64_t storage_free = 0;


        if (esp_vfs_fat_info(file::MOUNT_POINT_DATA, &storage_total, &storage_free) == ESP_OK) {
            addMemoryBar(storage_tab, file::MOUNT_POINT_DATA, storage_free, storage_total);
        }

        const auto sdcard_devices = hal::findDevices<hal::sdcard::SdCardDevice>(hal::Device::Type::SdCard);
        for (const auto& sdcard : sdcard_devices) {
            if (sdcard->isMounted() && esp_vfs_fat_info(sdcard->getMountPath().c_str(), &storage_total, &storage_free) == ESP_OK) {
                addMemoryBar(
                    storage_tab,
                    sdcard->getMountPath().c_str(),
                    storage_free,
                    storage_total
                );
            }
        }

        if (config::SHOW_SYSTEM_PARTITION) {
            if (esp_vfs_fat_info(file::MOUNT_POINT_SYSTEM, &storage_total, &storage_free) == ESP_OK) {
                addMemoryBar(storage_tab, file::MOUNT_POINT_SYSTEM, storage_free, storage_total);
            }
        }

#endif

#if configUSE_TRACE_FACILITY
        addRtosTasks(tasks_tab);
#endif

        addDevices(devices_tab);

        // Build info
        auto* tactility_version = lv_label_create(about_tab);
        lv_label_set_text_fmt(tactility_version, "Tactility v%s", TT_VERSION);
#ifdef ESP_PLATFORM
        auto* esp_idf_version = lv_label_create(about_tab);
        lv_label_set_text_fmt(esp_idf_version, "ESP-IDF v%d.%d.%d", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
#endif
    }
};

extern const AppManifest manifest = {
    .id = "SystemInfo",
    .name = "System Info",
    .icon = TT_ASSETS_APP_ICON_SYSTEM_INFO,
    .type = Type::System,
    .createApp = create<SystemInfoApp>
};

} // namespace

