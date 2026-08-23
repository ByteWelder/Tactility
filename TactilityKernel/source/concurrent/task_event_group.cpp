// SPDX-License-Identifier: Apache-2.0
#include <tactility/concurrent/task_event_group.h>

extern "C" {

void task_event_group_construct(TaskEventGroup* group) {
    event_group_construct(&group->internal.handle);
    mutex_construct(&group->internal.bit_mutex);
    group->claimed_bits = 0;
}

void task_event_group_destruct(TaskEventGroup* group) {
    event_group_destruct(&group->internal.handle);
    mutex_destruct(&group->internal.bit_mutex);
    group->claimed_bits = 0;
}

error_t task_event_group_claim_bit(TaskEventGroup* group, uint32_t* out_bit) {
    mutex_lock(&group->internal.bit_mutex);

    error_t result = ERROR_RESOURCE;
    for (uint32_t i = 0; i < TASK_EVENT_GROUP_MAX_BITS; i++) {
        uint32_t bit = 1u << i;
        if ((group->claimed_bits & bit) == 0) {
            group->claimed_bits |= bit;
            *out_bit = bit;
            result = ERROR_NONE;
            break;
        }
    }

    mutex_unlock(&group->internal.bit_mutex);
    return result;
}

void task_event_group_release_bit(TaskEventGroup* group, uint32_t bit) {
    // Clear the real flag, not just the claim bookkeeping: a signal nobody drained before
    // releasing (e.g. a courtesy nudge on unsubscribe with no one currently waiting) would
    // otherwise sit set in the underlying event group and hand the next claimant of this same
    // bit a false "already fired" wake.
    event_group_clear(group->internal.handle, bit);

    mutex_lock(&group->internal.bit_mutex);
    group->claimed_bits &= ~bit;
    mutex_unlock(&group->internal.bit_mutex);
}

error_t task_event_group_signal(TaskEventGroup* group, uint32_t bit) {
    return event_group_set(group->internal.handle, bit);
}

error_t task_event_group_wait(
    TaskEventGroup* group,
    uint32_t bits_mask,
    bool await_all,
    uint32_t* out_flags,
    TickType_t timeout
) {
    return event_group_wait(group->internal.handle, bits_mask, await_all, true, out_flags, timeout);
}

error_t task_event_group_wait_any(TaskEventGroup* group, uint32_t* out_flags, TickType_t timeout) {
    mutex_lock(&group->internal.bit_mutex);
    uint32_t mask = group->claimed_bits;
    mutex_unlock(&group->internal.bit_mutex);

    if (mask == 0) {
        // xEventGroupWaitBits() asserts on a zero mask - nothing claimed means nothing to wait
        // for, so this is the correct outcome anyway.
        return ERROR_TIMEOUT;
    }

    return task_event_group_wait(group, mask, false, out_flags, timeout);
}

} // extern "C"
