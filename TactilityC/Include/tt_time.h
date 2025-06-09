#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#define TT_TIMEZONE_NAME_BUFFER_LENGTH 32
#define TT_TIMEZONE_CODE_BUFFER_LENGTH 48

/**
 * Set the timezone
 * @param[in] name human-readable name
 * @param[in] code the technical code (from timezones.csv)
 */
void tt_timezone_set(const char* name, const char* code);

/**
 * Get the name of the timezone
 * @param[out] buffer the output buffer which will include a null terminator (should be TT_TIMEZONE_NAME_BUFFER_LENGTH)
 * @param[in] bufferSize the size of the output buffer
 */
bool tt_timezone_get_name(char* buffer, size_t bufferSize);

/**
 * Get the code of the timezone (see timezones.csv)
 */
bool tt_timezone_get_code(char* buffer, size_t bufferSize);

/** @return true when clocks should be shown as a 24 hours one instead of 12 hours */
bool tt_timezone_is_format_24_hour();

/** Set whether clocks should be shown as a 24 hours instead of 12 hours
 * @param[in] show24Hour
 */
void tt_timezone_set_format_24_hour(bool show24Hour);

#ifdef __cplusplus
}
#endif
