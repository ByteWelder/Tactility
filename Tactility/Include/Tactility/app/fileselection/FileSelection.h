#pragma once

#include <cstdint>
#include <string>

#include <app/stream.h>
#include <tactility/concurrent/task_event_group.h>

namespace tt::app::fileselection {

/**
 * Caller-owned storage for capturing the selected path back from the started file-selection
 * app's stdout. Must stay valid from startForExistingFile()/startForExistingOrNewFile() until
 * readResultPath()/closeResult() is called on it.
 */
struct PathResult {
    AppStream stream {};
    char buffer[512] {};
};

/**
 * Show a file selection dialog that allows the user to select an existing file, as a modal
 * child of @a callerAppInstanceId (see app_manager_start_for_result_with_streams()). Result
 * (0 = Ok, 1 = Cancelled) is delivered back via APP_EVENT_RESULT once this app's thread exits -
 * call readResultPath(result) right after receiving it on result == 0, or closeResult(result)
 * otherwise. The caller must call app_manager_stop() on the returned instance id once that event
 * arrives, to fully reap this instance.
 * @param[in,out] result caller-owned storage the selected path is captured into; see PathResult.
 * @param[in] eventGroup the caller's own event group, reused for the stream's readiness bits
 * (see app_stream_subscribe()) - the caller isn't required to actually wait on them itself.
 * @return the new app instance id
 */
uint32_t startForExistingFile(uint32_t callerAppInstanceId, PathResult& result, TaskEventGroup* eventGroup);

/**
 * Same as startForExistingFile(), but also allows picking a path that doesn't exist yet (for
 * "save as"-style flows).
 */
uint32_t startForExistingOrNewFile(uint32_t callerAppInstanceId, PathResult& result, TaskEventGroup* eventGroup);

/**
 * Reads the path captured in @a result and unsubscribes its stream. Call exactly once, after
 * APP_EVENT_RESULT arrives with result == 0 (Ok).
 */
std::string readResultPath(PathResult& result);

} // namespace
