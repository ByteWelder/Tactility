// SPDX-License-Identifier: Apache-2.0
// ESP-IDF only provides <sys/poll.h>. Apps include this through app-module's include path.
#pragma once

#if defined(__EMSCRIPTEN__)
// Emscripten's <sys/poll.h> is a redirect stub that includes <poll.h> back, which resolves
// here again and dead-ends with pollfd/nfds_t undefined. include_next skips past this
// shadow header to musl's real poll.h.
#include_next <poll.h>
#else
#include <sys/poll.h>
#endif
