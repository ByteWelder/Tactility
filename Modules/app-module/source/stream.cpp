// SPDX-License-Identifier: Apache-2.0
#include <app/stream.h>

#include <app/private/fd_table.h>
#include <app/private/ledger.h>
#include <app/private/stream_internal.h>

#include <tactility/concurrent/mutex.h>
#include <tactility/error.h>

namespace {

// Caller must hold stream->internal.mutex.
bool is_readable_locked(AppStream* stream) {
    return stream->internal.buffer.count > 0 || stream->internal.closed;
}

// Caller must hold stream->internal.mutex.
bool is_writable_locked(AppStream* stream) {
    return stream->internal.buffer.count < stream->internal.buffer.capacity || stream->internal.closed;
}

ssize_t stream_file_read(void* object, void* buffer, size_t size) {
    auto* stream = static_cast<AppStream*>(object);
    while (true) {
        if (app_stream_await(stream, APP_FILE_WAIT_READABLE, portMAX_DELAY) != ERROR_NONE) {
            return -1;
        }
        size_t read = app_stream_read(stream, buffer, size);
        if (read > 0) {
            return static_cast<ssize_t>(read);
        }
        mutex_lock(&stream->internal.mutex);
        bool is_closed = stream->internal.closed;
        mutex_unlock(&stream->internal.mutex);
        if (is_closed) {
            return 0; // EOF: readable-because-closed, and nothing left buffered
        }
        // Woke readable but another party drained it first. Re-await rather than assume EOF;
        // this is a defensive fallback since AppStream expects a single reader.
    }
}

ssize_t stream_file_write(void* object, const void* buffer, size_t size) {
    auto* stream = static_cast<AppStream*>(object);
    if (app_stream_await(stream, APP_FILE_WAIT_WRITABLE, portMAX_DELAY) != ERROR_NONE) {
        return -1;
    }
    mutex_lock(&stream->internal.mutex);
    bool is_closed = stream->internal.closed;
    mutex_unlock(&stream->internal.mutex);
    if (is_closed) {
        return -1; // broken pipe: woke writable only because the consumer closed
    }
    return static_cast<ssize_t>(app_stream_write(stream, buffer, size));
}

error_t stream_file_close(void* object) {
    return app_stream_close(static_cast<AppStream*>(object));
}

error_t stream_file_await(void* object, AppFileWait wait, TickType_t timeout) {
    return app_stream_await(static_cast<AppStream*>(object), wait, timeout);
}

uint32_t stream_file_poll(void* object) {
    auto* stream = static_cast<AppStream*>(object);
    mutex_lock(&stream->internal.mutex);
    uint32_t bits = 0;
    if (is_readable_locked(stream)) {
        bits |= APP_FILE_READABLE;
    }
    if (is_writable_locked(stream)) {
        bits |= APP_FILE_WRITABLE;
    }
    mutex_unlock(&stream->internal.mutex);
    return bits;
}

constexpr AppFileOps STREAM_OPS = {
    .read = stream_file_read,
    .write = stream_file_write,
    .close = stream_file_close,
    .await = stream_file_await,
    .poll = stream_file_poll,
};

} // namespace

extern "C" {

const AppFileOps* app_stream_ops(void) {
    return &STREAM_OPS;
}

error_t app_stream_subscribe(AppStream* stream, void* buffer, size_t buffer_capacity, TaskEventGroup* event_group, AppInstanceId producer_id, int producer_fd) {
    if (producer_fd < 0 || producer_fd >= APP_MAX_FDS) {
        return ERROR_OUT_OF_RANGE;
    }

    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    auto iterator = ledger.instances.find(producer_id);
    if (iterator == ledger.instances.end()) {
        mutex_unlock(&ledger.mutex);
        return ERROR_NOT_FOUND;
    }
    TaskHandle_t producer_task = iterator->second.task;
    AppFdTable* table = &iterator->second.fd_table;
    mutex_unlock(&ledger.mutex);

    uint32_t readable_bit;
    error_t claim_result = task_event_group_claim_bit(event_group, &readable_bit);
    if (claim_result != ERROR_NONE) {
        return claim_result;
    }
    uint32_t writable_bit;
    claim_result = task_event_group_claim_bit(event_group, &writable_bit);
    if (claim_result != ERROR_NONE) {
        task_event_group_release_bit(event_group, readable_bit);
        return claim_result;
    }

    stream->internal.producer_id = producer_id;
    stream->internal.producer_fd = producer_fd;
    stream->internal.producer_task = producer_task;
    stream->internal.event_group = event_group;
    stream->internal.readable_bit = readable_bit;
    stream->internal.writable_bit = writable_bit;
    stream->internal.buffer.data = static_cast<uint8_t*>(buffer);
    stream->internal.buffer.capacity = buffer_capacity;
    stream->internal.buffer.read_pos = 0;
    stream->internal.buffer.write_pos = 0;
    stream->internal.buffer.count = 0;
    stream->internal.closed = false;
    mutex_construct(&stream->internal.mutex);

    error_t bind_result = app_fd_table_bind(table, producer_fd, &STREAM_OPS, stream);
    if (bind_result != ERROR_NONE) {
        mutex_destruct(&stream->internal.mutex);
        task_event_group_release_bit(event_group, readable_bit);
        task_event_group_release_bit(event_group, writable_bit);
    }
    return bind_result;
}

error_t app_stream_unsubscribe(AppStream* stream) {
    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    auto iterator = ledger.instances.find(stream->internal.producer_id);
    AppFdTable* table = (iterator != ledger.instances.end()) ? &iterator->second.fd_table : nullptr;
    mutex_unlock(&ledger.mutex);

    if (table != nullptr) {
        AppFile current {};
        if (app_fd_table_get(table, stream->internal.producer_fd, &current) && current.object == stream) {
            app_fd_table_close(table, stream->internal.producer_fd); // -> stream_file_close() -> app_stream_close()
        }
    }

    app_stream_close(stream); // idempotent; covers the case where the producer already exited
    task_event_group_release_bit(stream->internal.event_group, stream->internal.readable_bit);
    task_event_group_release_bit(stream->internal.event_group, stream->internal.writable_bit);
    mutex_destruct(&stream->internal.mutex);
    return ERROR_NONE;
}

error_t app_stream_await(AppStream* stream, AppFileWait wait, TickType_t timeout) {
    mutex_lock(&stream->internal.mutex);
    bool ready = (wait == APP_FILE_WAIT_READABLE) ? is_readable_locked(stream) : is_writable_locked(stream);
    mutex_unlock(&stream->internal.mutex);
    if (ready) {
        return ERROR_NONE;
    }

    uint32_t bit = (wait == APP_FILE_WAIT_READABLE) ? stream->internal.readable_bit : stream->internal.writable_bit;
    return task_event_group_wait(stream->internal.event_group, bit, /*await_all=*/false, nullptr, timeout);
}

size_t app_stream_read(AppStream* stream, void* buffer, size_t buffer_size) {
    mutex_lock(&stream->internal.mutex);
    AppStreamBuffer& ring = stream->internal.buffer;
    size_t to_copy = buffer_size < ring.count ? buffer_size : ring.count;
    for (size_t i = 0; i < to_copy; i++) {
        static_cast<uint8_t*>(buffer)[i] = ring.data[(ring.read_pos + i) % ring.capacity];
    }
    if (ring.capacity > 0) {
        ring.read_pos = (ring.read_pos + to_copy) % ring.capacity;
    }
    ring.count -= to_copy;
    TaskEventGroup* event_group = stream->internal.event_group;
    uint32_t writable_bit = stream->internal.writable_bit;
    mutex_unlock(&stream->internal.mutex);
    if (to_copy > 0) {
        task_event_group_signal(event_group, writable_bit); // space freed up
    }
    return to_copy;
}

size_t app_stream_write(AppStream* stream, const void* buffer, size_t buffer_size) {
    mutex_lock(&stream->internal.mutex);
    AppStreamBuffer& ring = stream->internal.buffer;
    size_t available = ring.capacity - ring.count;
    size_t to_copy = buffer_size < available ? buffer_size : available;
    for (size_t i = 0; i < to_copy; i++) {
        ring.data[(ring.write_pos + i) % ring.capacity] = static_cast<const uint8_t*>(buffer)[i];
    }
    if (ring.capacity > 0) {
        ring.write_pos = (ring.write_pos + to_copy) % ring.capacity;
    }
    ring.count += to_copy;
    TaskEventGroup* event_group = stream->internal.event_group;
    uint32_t readable_bit = stream->internal.readable_bit;
    mutex_unlock(&stream->internal.mutex);
    if (to_copy > 0) {
        task_event_group_signal(event_group, readable_bit); // data became available
    }
    return to_copy;
}

error_t app_stream_close(AppStream* stream) {
    mutex_lock(&stream->internal.mutex);
    stream->internal.closed = true;
    TaskEventGroup* event_group = stream->internal.event_group;
    uint32_t readable_bit = stream->internal.readable_bit;
    uint32_t writable_bit = stream->internal.writable_bit;
    mutex_unlock(&stream->internal.mutex);
    task_event_group_signal(event_group, readable_bit);
    task_event_group_signal(event_group, writable_bit);
    return ERROR_NONE;
}

} // extern "C"
