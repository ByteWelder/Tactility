#pragma once

#include <Tactility/Mutex.h>
#include <Tactility/Thread.h>
#include <Tactility/Timer.h>

#include <lvgl.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tt::app::timezone {

/**
 * A searchable list of the time zones, with a button that detects the time zone from the location.
 * Its state survives rebuilding its widgets, e.g. when a window is covered and shown again.
 */
class TimeZoneView final {

public:

    /** Called with the LVGL lock held when the user picks a time zone, or when one was detected */
    using OnSelected = std::function<void(const std::string& name, const std::string& code)>;

    explicit TimeZoneView(OnSelected onSelected);

    /** Cancels a running detection and waits for its lookup to end, which can take a few seconds */
    ~TimeZoneView();

    TimeZoneView(const TimeZoneView&) = delete;
    TimeZoneView& operator=(const TimeZoneView&) = delete;

    /** Creates the widgets in @a parent, a flex column, with the LVGL lock held. The list grows to fill it. */
    void createWidgets(lv_obj_t* parent);

    /** Call with the LVGL lock held when the widgets were deleted */
    void destroyWidgets();

    /** Shows this time zone as the selected one */
    void setSelectedName(const std::string& name);

private:

    struct Entry {
        std::string name;
        std::string code;
    };

    OnSelected onSelected;
    Mutex mutex;
    // Guarded by mutex
    std::vector<Entry> entries;
    std::string filter;
    std::string selectedName;
    std::string message;
    std::unique_ptr<Timer> filterTimer;
    std::unique_ptr<Thread> detectThread;
    std::atomic<bool> detecting = false;
    // Set when the view is destroyed, so a running detection ends without using it
    std::atomic<bool> cancelled = false;

    // Valid while the widgets exist, guarded by the LVGL lock
    lv_obj_t* textarea = nullptr;
    lv_obj_t* detectButton = nullptr;
    lv_obj_t* detectSpinner = nullptr;
    lv_obj_t* messageLabel = nullptr;
    lv_obj_t* list = nullptr;

    static void onFilterChanged(lv_event_t* event);
    static void onDetectPressed(lv_event_t* event);
    static void onEntryPressed(lv_event_t* event);

    void loadEntries();
    void rebuildList();
    void showState();
    void detect();
    void select(const std::string& name, const std::string& code);
};

}
