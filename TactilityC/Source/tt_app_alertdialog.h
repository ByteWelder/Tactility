#pragma once

#include "tt_bundle.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void tt_app_alertdialog_start(const char* title, const char* message, const char* buttonLabels[], uint32_t buttonLabelCount);
int32_t tt_app_alertdialog_get_result_index(BundleHandle handle);

#ifdef __cplusplus
}
#endif