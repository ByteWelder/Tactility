#pragma once

#if TT_FEATURE_MESH

#include <tactility/error.h>

namespace tt::app::meshsettings {

/**
 * Join a hashtag channel (e.g. "#test") of the active mesh backend on the next free channel index.
 * Must not be called with the LVGL lock held.
 * @retval ERROR_INVALID_ARGUMENT for an empty name, or a name the backend can't derive a key for
 */
error_t joinHashtagChannel(const char* name);

}

#endif
