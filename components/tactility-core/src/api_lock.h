#pragma once

#include "core.h"

typedef EventFlag* ApiLock;

#define TT_API_LOCK_EVENT (1U << 0)

#define tt_api_lock_alloc_locked() tt_event_flag_alloc()

#define tt_api_lock_wait_unlock(_lock) \
    tt_event_flag_wait(_lock, TT_API_LOCK_EVENT, TtFlagWaitAny, TtWaitForever)

#define tt_api_lock_free(_lock) tt_event_flag_free(_lock)

#define tt_api_lock_unlock(_lock) tt_event_flag_set(_lock, TT_API_LOCK_EVENT)

#define tt_api_lock_wait_unlock_and_free(_lock) \
    tt_api_lock_wait_unlock(_lock);          \
    tt_api_lock_free(_lock);
