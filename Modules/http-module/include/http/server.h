// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <http/types.h>
#include <tactility/error.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque per-request handle; a handler only ever holds a pointer to one of these. */
struct HttpServerRequest;

/** @return ERROR_NONE if the request was handled; any other value is only logged, the response
 * (or its absence) is entirely up to the handler having already sent one via the
 * http_server_request_send*() functions below. */
typedef error_t (*HttpServerHandlerFn)(struct HttpServerRequest* request, void* user_ctx);

/**
 * One route: @a uri is matched against the request path (not the query string) together with
 * @a method. A trailing wildcard character matches by prefix (e.g. "/fs/" + wildcard matches
 * "/fs/list" and "/fs/x/y"); anything else must match exactly, same wildcard convention as
 * ESP-IDF's httpd_uri_match_wildcard(). @a uri is caller-owned and must outlive the server, same
 * contract as ESP-IDF's httpd_uri_t: a string literal is the usual case.
 */
struct HttpServerRequestHandler {
    const char* uri;
    enum HttpMethod method;
    HttpServerHandlerFn callback;
    void* user_ctx;
};

/** @a handlers is copied into the server at http_server_alloc() time; each handler's own `uri`
 * pointer is not, so it must still outlive the server. */
struct HttpServerConfig {
    uint16_t port;
    /** Bind address, e.g. "0.0.0.0". Caller-owned; only read during http_server_alloc(). */
    const char* address;
    /** Stack size in bytes for the server's own task, where the platform backend needs one. */
    uint32_t stack_size;
    const struct HttpServerRequestHandler* handlers;
    size_t handler_count;
};

struct HttpServer;

/**
 * Allocates a server for @a config; does not start listening yet, see http_server_start().
 * @return NULL on allocation failure
 */
struct HttpServer* http_server_alloc(const struct HttpServerConfig* config);

/** Stops the server if still running (see http_server_stop()) and frees it. */
void http_server_free(struct HttpServer* server);

/**
 * Starts listening and serving requests.
 * A request whose method+uri matches no registered handler gets a 404 response automatically.
 * @retval ERROR_NONE on success, including if the server was already started
 * @retval ERROR_RESOURCE the listening socket could not be created/bound
 */
error_t http_server_start(struct HttpServer* server);

/** Stops listening and blocks until any in-flight request has finished. Safe to call when not started. */
void http_server_stop(struct HttpServer* server);

bool http_server_is_started(struct HttpServer* server);

/** @return the bound port, e.g. to read back the OS-assigned port after starting with port 0. 0 if not started. */
uint16_t http_server_get_port(struct HttpServer* server);

// region Request

enum HttpMethod http_server_request_get_method(struct HttpServerRequest* request);

/**
 * Copies the request's path (not including the query string, e.g. "/fs/list") into @a buffer.
 * Useful from a handler registered against a wildcard route to see which concrete path matched.
 * @return the path's actual length, same truncation convention as http_server_request_get_query().
 */
size_t http_server_request_get_uri(struct HttpServerRequest* request, char* buffer, size_t buffer_size);

/**
 * Copies the request's raw query string (the part after '?', still URL-encoded, empty if none) into @a buffer.
 * @return the query string's actual length, regardless of @a buffer_size. Same truncation
 * convention as snprintf(): a return value >= @a buffer_size means the copy was truncated.
 */
size_t http_server_request_get_query(struct HttpServerRequest* request, char* buffer, size_t buffer_size);

/**
 * Copies the named header's value into @a buffer, case-insensitively.
 * @return the header value's actual length, same truncation convention as http_server_request_get_query(); 0 (with @a buffer left untouched) if the header is absent.
 */
size_t http_server_request_get_header(struct HttpServerRequest* request, const char* name, char* buffer, size_t buffer_size);

/** The request body's declared length (the "Content-Length" header), or 0 if absent. */
uint64_t http_server_request_get_content_length(struct HttpServerRequest* request);

/**
 * Reads up to @a buffer_size currently-available body bytes. Blocking: waits for at least one byte, up to an internal per-call timeout.
 * @return bytes read; 0 at end of body; negative on error or timeout
 */
int http_server_request_receive(struct HttpServerRequest* request, void* buffer, size_t buffer_size);

/** Must be called before the first http_server_request_send*() call on this request, if at all.
 * Defaults to 200. Has no effect once a response has started sending. */
void http_server_request_set_status(struct HttpServerRequest* request, status_code_t status_code);

/** Same timing as http_server_request_set_status(); defaults to "text/plain". */
void http_server_request_set_content_type(struct HttpServerRequest* request, const char* content_type);

/** Same timing as http_server_request_set_status(): adds one arbitrary response header.
 * e.g. "Location", "Content-Disposition".
 * Both @a name and @a value are copied.
 */
void http_server_request_set_header(struct HttpServerRequest* request, const char* name, const char* value);

/**
 * Sends the full response: status line, headers, then @a data as the entire body in one shot.
 * Only the first call to any http_server_request_send*()/send_chunk_start() for a given request has any effect.
 * @param[in] data may be NULL if @a length is 0
 */
error_t http_server_request_send(struct HttpServerRequest* request, const void* data, size_t length);

/** Same as http_server_request_send() with @a text's length and content type "text/plain". */
error_t http_server_request_send_string(struct HttpServerRequest* request, const char* text);

/** Sets @a status_code, then sends @a message as a plain-text body. */
error_t http_server_request_send_error(struct HttpServerRequest* request, int status_code, const char* message);

/**
 * Starts a chunked response: sends the status line and headers (no Content-Length; chunked
 * transfer instead) without a body yet. Follow with zero or more http_server_request_send_chunk() calls,
 * then exactly one http_server_request_send_chunk_end(). Useful for streaming a file whose size you don't
 * want to (or can't cheaply) compute up front. Only the first call to any
 * http_server_request_send*()/send_chunk_start() for a given request has any effect.
 */
error_t http_server_request_send_chunk_start(struct HttpServerRequest* request);

/** Sends one chunk of a response started with http_server_request_send_chunk_start(). */
error_t http_server_request_send_chunk(struct HttpServerRequest* request, const void* data, size_t length);

/** Terminates a chunked response started with http_server_request_send_chunk_start(). */
error_t http_server_request_send_chunk_end(struct HttpServerRequest* request);

// endregion

#ifdef __cplusplus
}
#endif
