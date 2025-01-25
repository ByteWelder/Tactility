#pragma once

#include <string>
#include <vector>
#include <dirent.h>
#include "Mutex.h"

namespace tt::app::files {

class State {

public:

    enum PendingAction {
        ActionNone,
        ActionDelete,
        ActionRename
    };

private:

    Mutex mutex = Mutex(Mutex::Type::Recursive);
    std::vector<dirent> dir_entries;
    std::string current_path;
    std::string selected_child_entry;
    PendingAction action = ActionNone;

public:


    State();

    void freeEntries() {
        dir_entries.clear();
    }

    ~State() {
        freeEntries();
    }

    bool setEntriesForChildPath(const std::string& child_path);
    bool setEntriesForPath(const std::string& path);

    const std::vector<dirent>& lockEntries() const {
        mutex.lock();
        return dir_entries;
    }

    void unlockEntries() {
        mutex.unlock();
    }

    bool getDirent(uint32_t index, dirent& dirent);

    void setSelectedChildEntry(const std::string& newFile) {
        selected_child_entry = newFile;
        action = ActionNone;
    }

    std::string getSelectedChildEntry() const { return selected_child_entry; }
    std::string getCurrentPath() const { return current_path; }

    std::string getSelectedChildPath() const;

    PendingAction getPendingAction() const {
        return action;
    }

    void setPendingAction(PendingAction newAction) {
        action = newAction;
    }
};

}
