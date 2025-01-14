#include "tt_app_alertdialog.h"
#include <app/alertdialog/AlertDialog.h>

extern "C" {

void tt_app_alertdialog_start(const char* title, const char* message, const char* buttonLabels[], uint32_t buttonLabelCount) {
    std::vector<std::string> list;
    for (int i = 0; i < buttonLabelCount; i++) {
        const char* item = buttonLabels[i];
        list.push_back(item);
    }
    tt::app::alertdialog::start(title, message, list);
}

int32_t tt_app_alertdialog_get_result_index(BundleHandle handle) {
    return tt::app::alertdialog::getResultIndex(*(tt::Bundle*)handle);
}

}
