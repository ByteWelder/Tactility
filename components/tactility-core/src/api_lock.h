#pragma once

#include "core.h"

typedef EventFlag* ApiLock;

#define API_LOCK_EVENT (1U << 0)

#define api_lock_alloc_locked() tt_event_flag_alloc()

#define api_lock_wait_unlock(_lock) \
    tt_event_flag_wait(_lock, API_LOCK_EVENT, TtFlagWaitAny, TtWaitForever)

#define api_lock_free(_lock) tt_event_flag_free(_lock)

#define api_lock_unlock(_lock) tt_event_flag_set(_lock, API_LOCK_EVENT)

#define api_lock_wait_unlock_and_free(_lock) \
    api_lock_wait_unlock(_lock);             \
    api_lock_free(_lock);
