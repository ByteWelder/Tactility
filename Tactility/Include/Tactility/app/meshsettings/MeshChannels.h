#pragma once


#include <tactility/error.h>

#include <cstddef>
#include <cstdint>

namespace tt::app::meshsettings {

/**
 * Join a hashtag channel (e.g. "#test") of the active mesh backend on the next free channel index.
 * Must not be called with the LVGL lock held.
 * @retval ERROR_INVALID_ARGUMENT for an empty name, or a name the backend can't derive a key for
 */
error_t joinHashtagChannel(const char* name);

/**
 * Create a private channel on the next free channel index of the active mesh backend, with a random key.
 * Must not be called with the LVGL lock held.
 * @retval ERROR_INVALID_ARGUMENT for an empty name
 */
error_t createPrivateChannel(const char* name);

/**
 * Join a private channel with a known key on the next free channel index of the active mesh backend.
 * Must not be called with the LVGL lock held.
 * @retval ERROR_INVALID_ARGUMENT for an empty name, or a key the backend doesn't accept
 */
error_t joinPrivateChannel(const char* name, const uint8_t* key, size_t keyLength);

}

