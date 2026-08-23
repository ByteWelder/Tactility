#include "doctest.h"

#include <tactility/concurrent/task_event_group.h>

TEST_CASE("task_event_group_wait_any wakes on any bit currently claimed in the group") {
    TaskEventGroup group {};
    task_event_group_construct(&group);

    uint32_t bit_a, bit_b;
    CHECK_EQ(task_event_group_claim_bit(&group, &bit_a), ERROR_NONE);
    CHECK_EQ(task_event_group_claim_bit(&group, &bit_b), ERROR_NONE);
    CHECK_NE(bit_a, bit_b);

    CHECK_EQ(task_event_group_signal(&group, bit_b), ERROR_NONE);

    uint32_t out_flags = 0;
    CHECK_EQ(task_event_group_wait_any(&group, &out_flags, 0), ERROR_NONE);
    CHECK_EQ(out_flags, bit_b);

    task_event_group_destruct(&group);
}

TEST_CASE("task_event_group_wait_any times out immediately when the group has no claimed bits") {
    TaskEventGroup group {};
    task_event_group_construct(&group);

    CHECK_EQ(task_event_group_wait_any(&group, nullptr, 0), ERROR_TIMEOUT);

    task_event_group_destruct(&group);
}

TEST_CASE("task_event_group_wait_any times out when claimed bits exist but none are signalled") {
    TaskEventGroup group {};
    task_event_group_construct(&group);

    uint32_t bit;
    CHECK_EQ(task_event_group_claim_bit(&group, &bit), ERROR_NONE);

    CHECK_EQ(task_event_group_wait_any(&group, nullptr, 0), ERROR_TIMEOUT);

    task_event_group_destruct(&group);
}
