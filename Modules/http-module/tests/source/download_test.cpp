#include "doctest.h"

#include <http/download.h>

TEST_CASE("http_download_start requires a subscribed subscription") {
    HttpDownloadSubscription sub {};
    CHECK_EQ(http_download_start("http://example.com/file", "/cert.pem", "/target.bin", &sub), ERROR_INVALID_ARGUMENT);
}

TEST_CASE("http_download_poll times out before the download finishes") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    HttpDownloadSubscription sub {};
    CHECK_EQ(http_download_subscribe(&sub, &event_group), ERROR_NONE);

    HttpDownloadEvent event {};
    CHECK_EQ(http_download_poll(&sub, &event), ERROR_TIMEOUT);

    http_download_unsubscribe(&sub);
    task_event_group_destruct(&event_group);
}

TEST_CASE("http_download_start on this (non-ESP-IDF) platform always fails the download") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    HttpDownloadSubscription sub {};
    CHECK_EQ(http_download_subscribe(&sub, &event_group), ERROR_NONE);
    CHECK_EQ(http_download_start("http://example.com/file", "/cert.pem", "/target.bin", &sub), ERROR_NONE);

    CHECK_EQ(task_event_group_wait(&event_group, sub.bit, false, nullptr, pdMS_TO_TICKS(2000)), ERROR_NONE);

    HttpDownloadEvent event {};
    CHECK_EQ(http_download_poll(&sub, &event), ERROR_NONE);
    CHECK_EQ(event.type, HTTP_DOWNLOAD_EVENT_ERROR);

    http_download_unsubscribe(&sub);
    task_event_group_destruct(&event_group);
}

TEST_CASE("http_download_unsubscribe reports ERROR_NOT_FOUND when not subscribed") {
    HttpDownloadSubscription sub {};
    CHECK_EQ(http_download_unsubscribe(&sub), ERROR_NOT_FOUND);
}

TEST_CASE("http_download_cancel reports ERROR_INVALID_STATE when not subscribed") {
    HttpDownloadSubscription sub {};
    CHECK_EQ(http_download_cancel(&sub), ERROR_INVALID_STATE);
}

TEST_CASE("http_download_unsubscribe is safe to call right after start, without waiting for the download to finish") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    HttpDownloadSubscription sub {};
    CHECK_EQ(http_download_subscribe(&sub, &event_group), ERROR_NONE);
    CHECK_EQ(http_download_start("http://example.com/file", "/cert.pem", "/target.bin", &sub), ERROR_NONE);

    // No cancel, no poll for the terminal event - just unsubscribe and tear down immediately,
    // racing whatever the download task is doing.
    CHECK_EQ(http_download_unsubscribe(&sub), ERROR_NONE);
    CHECK_EQ(http_download_unsubscribe(&sub), ERROR_NOT_FOUND);

    // The bit is free again immediately, even though the download task may still be running.
    uint32_t reclaimed_bit;
    CHECK_EQ(task_event_group_claim_bit(&event_group, &reclaimed_bit), ERROR_NONE);
    task_event_group_release_bit(&event_group, reclaimed_bit);

    task_event_group_destruct(&event_group);
}

TEST_CASE("http_download_cancel before start makes the download finish as cancelled") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    HttpDownloadSubscription sub {};
    CHECK_EQ(http_download_subscribe(&sub, &event_group), ERROR_NONE);
    CHECK_EQ(http_download_cancel(&sub), ERROR_NONE);
    CHECK_EQ(http_download_start("http://example.com/file", "/cert.pem", "/target.bin", &sub), ERROR_NONE);

    CHECK_EQ(task_event_group_wait(&event_group, sub.bit, false, nullptr, pdMS_TO_TICKS(2000)), ERROR_NONE);

    HttpDownloadEvent event {};
    CHECK_EQ(http_download_poll(&sub, &event), ERROR_NONE);
    CHECK_EQ(event.type, HTTP_DOWNLOAD_EVENT_CANCELLED);

    http_download_unsubscribe(&sub);
    task_event_group_destruct(&event_group);
}
