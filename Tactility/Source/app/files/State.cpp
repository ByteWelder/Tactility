#include "State.h"
#include "kernel/Kernel.h"
#include "Log.h"
#include "FileUtils.h"
#include "Partitions.h"
#include "hal/SdCard.h"

#include <unistd.h>

#define TAG "files_app"

namespace tt::app::files {

State::State() {
    if (kernel::getPlatform() == kernel::PlatformSimulator) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            setEntriesForPath(cwd);
        } else {
            TT_LOG_E(TAG, "Failed to get current work directory files");
            setEntriesForPath("/");
        }
    } else {
        setEntriesForPath("/");
    }
}

std::string State::getSelectedChildPath() const {
    return getChildPath(current_path, selected_child_entry);
}

bool State::setEntriesForPath(const std::string& path) {
    auto scoped_lock = mutex.scoped();
    if (!scoped_lock->lock(100)) {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "setEntriesForPath");
        return false;
    }

    TT_LOG_I(TAG, "Changing path: %s -> %s", current_path.c_str(), path.c_str());

    /**
     * ESP32 does not have a root directory, so we have to create it manually.
     * We'll add the NVS Flash partitions and the binding for the sdcard.
     */
#if TT_SCREENSHOT_MODE
    bool show_custom_root = true;
#else
    bool show_custom_root = (kernel::getPlatform() == kernel::PlatformEsp) && (path == "/");
#endif
    if (show_custom_root) {
        TT_LOG_I(TAG, "Setting custom root");
        dir_entries.clear();
        dir_entries.push_back({
            .d_ino = 0,
            .d_type = TT_DT_DIR,
            .d_name = SYSTEM_PARTITION_NAME
        });
        dir_entries.push_back({
            .d_ino = 1,
            .d_type = TT_DT_DIR,
            .d_name = DATA_PARTITION_NAME
        });
        dir_entries.push_back({
            .d_ino = 2,
            .d_type = TT_DT_DIR,
            .d_name = TT_SDCARD_MOUNT_NAME
        });

        current_path = path;
        selected_child_entry = "";
        action = ActionNone;
        return true;
    } else {
        dir_entries.clear();
        int count = tt::app::files::scandir(path, dir_entries, &dirent_filter_dot_entries, dirent_sort_alpha_and_type);
        if (count >= 0) {
            TT_LOG_I(TAG, "%s has %u entries", path.c_str(), count);
            current_path = path;
            selected_child_entry = "";
            action = ActionNone;
            return true;
        } else {
            TT_LOG_E(TAG, "Failed to fetch entries for %s", path.c_str());
            return false;
        }
    }
}

bool State::setEntriesForChildPath(const std::string& child_path) {
    auto path = getChildPath(current_path, child_path);
    TT_LOG_I(TAG, "Navigating from %s to %s", current_path.c_str(), path.c_str());
    return setEntriesForPath(path);
}

bool State::getDirent(uint32_t index, dirent& dirent) {
    auto scoped_mutex = mutex.scoped();
    if (!scoped_mutex->lock(50 / portTICK_PERIOD_MS)) {
        return false;
    }

    if (index < dir_entries.size()) {
        dirent = dir_entries[index];
        return true;
    } else {
        return false;
    }
}
}
