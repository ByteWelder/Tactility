#pragma once

#include <Tactility/Mutex.h>

#include <string>
#include <vector>
#include <dirent.h>

namespace tt::app::files {

class State final {

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

    template <std::invocable<const std::vector<dirent> &> Func>
    void withEntries(Func&& onEntries) const {
        mutex.withLock([&] {
            std::invoke(std::forward<Func>(onEntries), dir_entries);
        });
    }

    bool getDirent(uint32_t index, dirent& dirent);

    void setSelectedChildEntry(const std::string& newFile) {
        selected_child_entry = newFile;
        action = ActionNone;
    }

    std::string getSelectedChildEntry() const { return selected_child_entry; }
    std::string getCurrentPath() const { return current_path; }

    std::string getSelectedChildPath() const;

    PendingAction getPendingAction() const { return action; }

    void setPendingAction(PendingAction newAction) { action = newAction; }
};

}
