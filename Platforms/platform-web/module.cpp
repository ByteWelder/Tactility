// SPDX-License-Identifier: Apache-2.0
#include <tactility/module.h>

error_t platform_posix_start_partitions();

extern "C" {

Module platform_web_module = {
    .name = "platform-web",
    .start = platform_posix_start_partitions,
    .stop = nullptr,
    .drivers = nullptr,
    .symbols = nullptr,
    .internal = nullptr,
};

}
