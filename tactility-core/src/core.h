#pragma once

#include <stdlib.h>

#include "tactility_core.h"

#include "event_flag.h"
#include "kernel.h"
#include "message_queue.h"
#include "mutex.h"
#include "pubsub.h"
#include "semaphore.h"
#include "string.h"
#include "thread.h"
#include "timer.h"
#include "tt_stream_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

void tt_core_init();

#ifdef __cplusplus
}
#endif
