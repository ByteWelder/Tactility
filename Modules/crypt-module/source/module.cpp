// SPDX-License-Identifier: Apache-2.0
#include <crypt/crypt.h>
#include <crypt/hash.h>
#include <crypt/module.h>

extern "C" {

static const ModuleSymbol SYMBOLS[] = {
    DEFINE_MODULE_SYMBOL(crypt_get_iv),
    DEFINE_MODULE_SYMBOL(crypt_generate_iv),
    DEFINE_MODULE_SYMBOL(crypt_encrypt),
    DEFINE_MODULE_SYMBOL(crypt_decrypt),
    DEFINE_MODULE_SYMBOL(djb2_str),
    DEFINE_MODULE_SYMBOL(djb2_data),
    MODULE_SYMBOL_TERMINATOR
};

Module crypt_module = {
    .name = "crypt",
    .start = nullptr,
    .stop = nullptr,
    .drivers = nullptr,
    .symbols = SYMBOLS,
    .internal = nullptr,
};

}
