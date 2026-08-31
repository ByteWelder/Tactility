// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Fixed capacity of one queued log line (256 bytes * 24-deep queue = 6KB, negligible against the
// hundreds of KB of free heap typically available - generous enough to hold a full log line with
// color/timestamp/tag prefix plus a realistic message, e.g. a full SD-card path in an error log).
#define LOG_QUEUE_MESSAGE_MAX_LENGTH 256U

/**
 * Starts the shared log queue and its dedicated drain (writer) task. Idempotent - safe to call
 * more than once (only the first call has any effect). Must be called as early as possible in
 * boot - see kernel_init.cpp, which calls this as its very first statement.
 */
void log_queue_init(void);

/**
 * Enqueues an already-fully-formatted, ready-to-write log line (NOT a printf-style format
 * string) so the dedicated drain task can write it via a path that never goes through
 * app-module's fd-table redirection (see Modules/app-module/source/io.cpp's app_io_write()) -
 * this is the ONLY function log-producing code should call to reach the console; it must never
 * itself call write()/printf()/etc, since those would be intercepted whenever called from an app
 * instance's own task, which is exactly the bug this exists to structurally prevent.
 *
 * Non-blocking: if the queue is full, the message is dropped and a counter is incremented; the
 * drain task prepends a "N messages dropped" notice before its next write. If called before
 * log_queue_init() has run, performs a direct synchronous write instead of enqueueing - safe,
 * since no app instance (and so no fd-table redirection) can exist yet that early in boot.
 * Truncates messages longer than LOG_QUEUE_MESSAGE_MAX_LENGTH - 1.
 */
void log_queue_write(const char* data, size_t length);

#ifdef __cplusplus
}
#endif
