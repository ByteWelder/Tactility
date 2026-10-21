// SPDX-License-Identifier: Apache-2.0
#include <tactility/driver.h>
#include <tactility/error.h>
#include <tactility/module.h>

// Deliberately not linked against the test binary: this stays undefined until dlopen() resolves it
// against the loading process, which records the module's lifecycle.
extern "C" void module_fixture_event(bool started);

namespace {

error_t start() {
    module_fixture_event(true);
    return ERROR_NONE;
}

error_t stop() {
    module_fixture_event(false);
    return ERROR_NONE;
}

// Lets tests bind a device to the module, which makes it in use
Driver driver = {
    .name = "module-fixture-driver",
    .compatible = (const char*[]) { "module-fixture", nullptr },
};

Driver* const drivers[] = { &driver, nullptr };

Module module = {
    .name = "module-fixture",
    .start = start,
    .stop = stop,
    .drivers = drivers,
    .symbols = nullptr,
    .internal = nullptr,
};

} // namespace

// A module's main() returns its Module. Clang rejects a main() that doesn't return int, so it's declared under another name.
extern "C" Module* module_main(int, char**) __asm__("main");

extern "C" Module* module_main(int, char**) {
    return &module;
}
