#include "TactilityC/app/SelectionDialog.h"

int main(int argc, char* argv[])
{
    const char* items[] = {
        "Yes",
        "Absolutely"
    };

    tt_app_selectiondialog_start("External apps are awesome!", 2, items);

    return 0;
}
