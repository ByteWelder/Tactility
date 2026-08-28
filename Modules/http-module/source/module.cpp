// SPDX-License-Identifier: Apache-2.0
#include <http/download.h>
#include <http/module.h>

extern "C" {

static const ModuleSymbol http_module_symbols[] = {
    DEFINE_MODULE_SYMBOL(http_download_subscribe),
    DEFINE_MODULE_SYMBOL(http_download_unsubscribe),
    DEFINE_MODULE_SYMBOL(http_download_poll),
    DEFINE_MODULE_SYMBOL(http_download_start),
    DEFINE_MODULE_SYMBOL(http_download_cancel),
    MODULE_SYMBOL_TERMINATOR
};

Module http_module = {
    .name = "http",
    .start = nullptr,
    .stop = nullptr,
    .drivers = nullptr,
    .symbols = http_module_symbols,
    .internal = nullptr,
};

}
