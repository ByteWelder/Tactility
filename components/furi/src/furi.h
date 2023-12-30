#pragma once

#include <stdlib.h>

#include "furi_core.h"

#include "furi_string.h"
#include "event_flag.h"
#include "kernel.h"
#include "message_queue.h"
#include "mutex.h"
#include "pubsub.h"
#include "record.h"
#include "semaphore.h"
#include "stream_buffer.h"
#include "string.h"
#include "thread.h"
#include "timer.h"

#ifdef __cplusplus
extern "C" {
#endif

void furi_init();

#ifdef __cplusplus
}
#endif
