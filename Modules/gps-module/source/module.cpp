// SPDX-License-Identifier: Apache-2.0
#include <gps/module.h>
#include <gps/private/gps_ledger.h>

#include <tactility/error.h>
#include <tactility/module.h>

extern "C" {

static error_t start() {
    // Materializes devices for configurations persisted in previous sessions.
    gps_ledger_sync();
    return ERROR_NONE;
}

static error_t stop() {
    gps_ledger_clear();
    return ERROR_NONE;
}

Module gps_module = {
    .name = "gps",
    .start = start,
    .stop = stop,
    .drivers = nullptr,
    .symbols = nullptr,
    .internal = nullptr
};

}
