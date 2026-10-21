// SPDX-License-Identifier: Apache-2.0
// Latched one-shot/resumable event used to park and resume FreeRTOS task pthreads
// in the Emscripten port. Derived from FreeRTOS-Kernel's
// portable/ThirdParty/GCC/Posix/utils/wait_for_event (MIT), with the robust-mutex
// handling removed: Emscripten's pthread support does not need it, and a task
// pthread is only ever cancelled while it is waiting on its own event, which does
// not hold this mutex.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

struct event;

struct event* event_create(void);
void event_delete(struct event* ev);
/** Blocks until event_signal() has been called; consumes the latched signal. */
bool event_wait(struct event* ev);
/** Latches the event and wakes one waiter. Signals are not counted: signalling an
 *  already-signalled event is a no-op until it is waited on. */
void event_signal(struct event* ev);

#ifdef __cplusplus
}
#endif
