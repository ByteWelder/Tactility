#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <app/stream.h>
#include <tactility/concurrent/task_event_group.h>

/**
 * Show a dialog with a title, a message and a text field.
 */
namespace tt::app::inputdialog {

/**
 * Show a dialog with the provided title, message and prefilled text, as a modal child of
 * @a callerAppInstanceId (see app_start_for_result_with_streams()). Result (0 = OK, 1 =
 * Cancelled or dismissed without a press) is delivered back via APP_EVENT_RESULT once this app's
 * thread exits. On result == 0, read the entered text with app_stream_read(&stream, ...) then
 * app_stream_unsubscribe(&stream); on any other result, just app_stream_unsubscribe(&stream).
 * The caller must call app_manager_stop() on the returned instance id once that event arrives,
 * to fully reap this instance.
 * @param[in,out] stream caller-owned storage bound to the started app's stdout; must stay valid
 * until app_stream_unsubscribe() is called on it (see above).
 * @param[in] buffer caller-owned backing storage for @a stream's ring buffer; must stay valid
 * for the same duration as @a stream.
 * @param[in] bufferCapacity size of @a buffer in bytes.
 * @param[in] eventGroup the caller's own event group, reused for the stream's readiness bits
 * (see app_stream_subscribe()). The caller isn't required to actually wait on them itself.
 * @return the new dialog's app instance id
 */
uint32_t start(uint32_t callerAppInstanceId, const std::string& title, const std::string& message, const std::string& prefilled, AppStream& stream, void* buffer, size_t bufferCapacity, TaskEventGroup* eventGroup);

}
