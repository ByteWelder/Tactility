#include "tt_app_selectiondialog.h"
#include <app/selectiondialog/SelectionDialog.h>

extern "C" {

void tt_app_selectiondialog_start(const char* title, int argc, const char* argv[]) {
    std::vector<std::string> list;
    for (int i = 0; i < argc; i++) {
        const char* item = argv[i];
        list.push_back(item);
    }
    tt::app::selectiondialog::start(title, list);
}

int32_t tt_app_selectiondialog_get_result_index(BundleHandle handle) {
    return tt::app::selectiondialog::getResultIndex(*(tt::Bundle*)handle);
}

}
