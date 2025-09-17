#pragma once

#include <Tactility/Mutex.h>

#include <string>
#include <vector>
#include <dirent.h>

namespace tt::app::fileselection {

class State final {

    Mutex mutex = Mutex(Mutex::Type::Recursive);
    std::vector<dirent> dir_entries;
    std::string current_path;
    std::string selected_child_entry;

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
        mutex.withLock([&]() {
            std::invoke(std::forward<Func>(onEntries), dir_entries);
        });
    }

    bool getDirent(uint32_t index, dirent& dirent);

    void setSelectedChildEntry(const std::string& newFile) {
        selected_child_entry = newFile;
    }

    std::string getSelectedChildEntry() const { return selected_child_entry; }
    std::string getCurrentPath() const { return current_path; }
    std::string getCurrentPathWithTrailingSlash() const {
        if (current_path.length() > 1) {
            return current_path + "/";
        } else {
            return current_path;
        }
    }

    std::string getSelectedChildPath() const;
};

}
