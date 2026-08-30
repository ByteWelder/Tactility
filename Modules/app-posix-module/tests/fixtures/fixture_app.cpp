// SPDX-License-Identifier: Apache-2.0
#include <app/scheduler.h>

#include <cstdint>

// Deliberately not linked against app-module: app_scheduler_current_app_id() stays an undefined
// symbol in this .so, resolved at dlopen() time against the loading process's own copy. Proves
// app-posix-module's loader lets a loaded app call straight back into Tactility without linking
// its own copy of it.
extern "C" int32_t main(int, char*[]) {
    return static_cast<int32_t>(app_scheduler_current_app_id());
}
