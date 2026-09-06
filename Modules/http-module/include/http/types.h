// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tactility/error.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// esp_http_client.h defines the same unscoped HTTP_METHOD_GET/POST/PUT/DELETE names.
// module.cpp avoids the clash by forward-declaring this enum instead of including this header.
// The fixed underlying type (C++ only) is what makes that forward declaration legal.
#ifdef __cplusplus
enum HttpMethod : int {
#else
enum HttpMethod {
#endif
    HTTP_METHOD_CONNECT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_GET,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_POST,
    HTTP_METHOD_PATCH,
    HTTP_METHOD_PUT,
    HTTP_METHOD_TRACE,
};

/** An HTTP response status code, e.g. 200 or 404. */
typedef uint16_t status_code_t;

/** @return @a method's wire form, e.g. "GET" for HTTP_METHOD_GET. */
const char* http_method_to_string(enum HttpMethod method);

/**
 * Parses @a text (e.g. the method token off a request line) into @a out_method.
 * @retval ERROR_NONE on success
 * @retval ERROR_NOT_FOUND @a text does not match any HttpMethod
 */
error_t http_method_from_string(const char* text, enum HttpMethod* out_method);

/**
 * @warning Not all status codes are implemented, so check the return value
 * @param[out] text @a code's standard reason phrase, e.g. "OK" for 200; only set on success
 * @retval ERROR_NONE on success
 * @retval ERROR_NOT_FOUND @a code has no known reason phrase
 */
error_t status_code_to_string(status_code_t code, const char** text);

#ifdef __cplusplus
}
#endif
