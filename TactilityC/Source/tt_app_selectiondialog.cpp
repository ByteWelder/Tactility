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

}
