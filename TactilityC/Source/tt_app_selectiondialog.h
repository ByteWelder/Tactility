#pragma once

#include "tt_bundle.h"

#ifdef __cplusplus
extern "C" {
#endif

void tt_app_selectiondialog_start(const char* title, int argc, const char* argv[]);

int32_t tt_app_selectiondialog_get_result_index(BundleHandle handle);

#ifdef __cplusplus
}
#endif