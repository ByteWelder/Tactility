#include "Tactility/lvgl/Toolbar.h"

#include <Tactility/Assets.h>
#include <Tactility/hal/Device.h>
#include <Tactility/Tactility.h>

#include <lvgl.h>

namespace tt::app::systeminfo {

#define TAG "system_info"

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

static void addMemoryBar(lv_obj_t* parent, const char* label, size_t used, size_t total) {
    auto* container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);

    auto* left_label = lv_label_create(container);
    lv_label_set_text(left_label, label);
    lv_obj_set_width(left_label, 60);

    auto* bar = lv_bar_create(container);
    lv_obj_set_flex_grow(bar, 1);

    if (total > 0) {
        lv_bar_set_range(bar, 0, (int32_t)total);
    } else {
        lv_bar_set_range(bar, 0, 1);
    }

    lv_bar_set_value(bar, (int32_t)used, LV_ANIM_OFF);

    auto* bottom_label = lv_label_create(parent);
    lv_label_set_text_fmt(bottom_label, "%u / %u kB", (used / 1024), (total / 1024));
    lv_obj_set_width(bottom_label, LV_PCT(100));
    lv_obj_set_style_text_align(bottom_label, LV_TEXT_ALIGN_RIGHT, 0);
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

class SystemInfoApp : public App {

    void onShow(AppContext& app, lv_obj_t* parent) override {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        lvgl::toolbar_create(parent, app);

        // This wrapper automatically has its children added vertically underneath eachother
        auto* wrapper = lv_obj_create(parent);
        lv_obj_set_style_border_width(wrapper, 0, 0);
        lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_width(wrapper, LV_PCT(100));
        lv_obj_set_flex_grow(wrapper, 1);

        // Wrapper for the memory usage bars
        auto* memory_label = lv_label_create(wrapper);
        lv_label_set_text(memory_label, "Memory usage");
        auto* memory_wrapper = lv_obj_create(wrapper);
        lv_obj_set_flex_flow(memory_wrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_size(memory_wrapper, LV_PCT(100), LV_SIZE_CONTENT);

        addMemoryBar(memory_wrapper, "Internal", getHeapTotal() - getHeapFree(), getHeapTotal());
        if (getSpiTotal() > 0) {
            addMemoryBar(memory_wrapper, "External", getSpiTotal() - getSpiFree(), getSpiTotal());
        }

#if configUSE_TRACE_FACILITY
        auto* tasks_label = lv_label_create(wrapper);
        lv_label_set_text(tasks_label, "Tasks");
        auto* tasks_wrapper = lv_obj_create(wrapper);
        lv_obj_set_flex_flow(tasks_wrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_size(tasks_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        addRtosTasks(tasks_wrapper);
#endif
        auto* devices_label = lv_label_create(wrapper);
        lv_label_set_text(devices_label, "Devices");
        auto* devices_wrapper = lv_obj_create(wrapper);
        lv_obj_set_flex_flow(devices_wrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_size(devices_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        addDevices(devices_wrapper);

#ifdef ESP_PLATFORM
        // Build info
        auto* build_info_label = lv_label_create(wrapper);
        lv_label_set_text(build_info_label, "Build info");
        auto* build_info_wrapper = lv_obj_create(wrapper);
        lv_obj_set_flex_flow(build_info_wrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_size(build_info_wrapper, LV_PCT(100), LV_SIZE_CONTENT);

        auto* esp_idf_version = lv_label_create(build_info_wrapper);
        lv_label_set_text_fmt(esp_idf_version, "IDF version: %d.%d.%d", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
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

