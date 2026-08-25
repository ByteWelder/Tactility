// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <tactility/concurrent/event_group.h>
#include <tactility/concurrent/mutex.h>
#include <tactility/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Usable bits per FreeRTOS event group when configUSE_16_BIT_TICKS is 0
 * (This project's setting on every target. See Devices/simulator/Source/FreeRTOSConfig.h). */
#define TASK_EVENT_GROUP_MAX_BITS 24

/**
 * Lets a task block on several independent event sources (e.g. app_event and system_event) with
 * a single wait call, instead of each source claiming the task's own FreeRTOS notification value
 * or its own semaphore - primitives that can't be combined into one blocking call.
 *
 * Caller-owned storage, constructed/destructed like struct Mutex/EventGroupHandle_t - no heap
 * allocation. Each event source claims its own bit at subscribe time via
 * task_event_group_claim_bit(), so sources at any layer (kernel, firmware, app) can share one
 * group without knowing about each other's bit assignments.
 *
 * @warning `bit_mutex`/`claimed_bits` are implementation-only; do not read or write directly.
 */
struct TaskEventGroup {
    uint32_t claimed_bits;
    struct {
        EventGroupHandle_t handle;
        struct Mutex bit_mutex;
    } internal;
};

/** @warning Does not work in ISR context. */
void task_event_group_construct(struct TaskEventGroup* group);

/** @warning Does not work in ISR context. */
void task_event_group_destruct(struct TaskEventGroup* group);

/**
 * Claim an unused bit in @a group for exclusive use by one event source. Always starts clear
 * (unsignalled), whether this bit has ever been claimed before or not.
 * @param[out] out_bit set to the claimed bit (a single-bit mask, not a bit index) on success
 * @retval ERROR_NONE on success
 * @retval ERROR_RESOURCE all TASK_EVENT_GROUP_MAX_BITS bits are already claimed
 */
error_t task_event_group_claim_bit(struct TaskEventGroup* group, uint32_t* out_bit);

/**
 * Release a bit previously returned by task_event_group_claim_bit(), making it available for
 * reuse. Also clears it, so a signal nobody drained before releasing can't be handed to the next
 * claimant as a false "already fired" wake.
 */
void task_event_group_release_bit(struct TaskEventGroup* group, uint32_t bit);

/** Signal @a bit, waking any task blocked in task_event_group_wait() for it. ISR-safe. */
error_t task_event_group_signal(struct TaskEventGroup* group, uint32_t bit);

/**
 * Block until one or more bits in @a bits_mask are signalled, or @a timeout elapses. Matched
 * bits are cleared on exit.
 * @param[in] await_all if true, wait for every bit in @a bits_mask; otherwise wait for any of them
 * @param[out] out_flags if non-NULL, set to the matched bits on ERROR_NONE
 */
error_t task_event_group_wait(
    struct TaskEventGroup* group,
    uint32_t bits_mask,
    bool await_all,
    uint32_t* out_flags,
    TickType_t timeout
);

/**
 * Like task_event_group_wait(), but waits on every bit currently claimed in @a group (snapshotted
 * at call time) instead of a caller-supplied mask - so the caller doesn't need to track/OR
 * together each subscription's bit by hand. A bit claimed by a new subscription *during* the wait
 * isn't included until the next call.
 * @warning Meant for the common case where every subscription sharing @a group is set up before
 * the wait loop starts (matching app_event/system_event/wifi_event's own usage patterns) - not
 * for a group a subscriber can join mid-wait and expect to wake immediately.
 * @retval ERROR_TIMEOUT returned immediately, without blocking, if @a group currently has no
 * claimed bits (FreeRTOS asserts on a zero-bit wait mask, so this is handled before it gets there)
 */
error_t task_event_group_wait_any(struct TaskEventGroup* group, uint32_t* out_flags, TickType_t timeout);

#ifdef __cplusplus
}
#endif
