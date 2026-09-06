// SPDX-License-Identifier: Apache-2.0
#include <http/server.h>

#include <tactility/concurrent/mutex.h>
#include <tactility/error.h>
#include <tactility/freertos/semphr.h>
#include <tactility/freertos/task.h>
#include <tactility/log.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr auto* TAG = "http-server";

constexpr uint32_t DEFAULT_STACK_SIZE = 5120;
constexpr int LISTEN_BACKLOG = 8;
// How often the accept loop wakes to re-check stop_requested; not a per-request timeout.
constexpr int ACCEPT_POLL_TIMEOUT_MS = 200;
// Applied to every accepted connection's socket, for both header/body reads.
constexpr int CONNECTION_RECEIVE_TIMEOUT_MS = 5000;
// Total wall-clock budget for one connection's request line + headers, independent of the
// per-recv timeout above: a client trickling one byte at a time never trips that timeout but
// would otherwise stall the single-threaded accept loop (and http_server_stop()) indefinitely.
constexpr int CONNECTION_TOTAL_TIMEOUT_MS = 10000;
constexpr size_t MAX_LINE_LENGTH = 8192;
constexpr size_t MAX_HEADER_COUNT = 32;

// The FreeRTOS POSIX port's tick signal can interrupt a blocking syscall on the thread it targets
// (observed on the socket calls below), so every blocking recv()/send() here retries on EINTR
// rather than treating it as a real error or an orderly close.
ssize_t receive_retry(int socket_fd, void* buffer, size_t size) {
    ssize_t result;
    do {
        result = recv(socket_fd, buffer, size, 0);
    } while (result < 0 && errno == EINTR);
    return result;
}

ssize_t send_retry(int socket_fd, const void* buffer, size_t size) {
    size_t total_sent = 0;
    while (total_sent < size) {
        ssize_t sent = send(socket_fd, static_cast<const char*>(buffer) + total_sent, size - total_sent, 0);
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            return sent;
        }
        total_sent += static_cast<size_t>(sent);
    }
    return static_cast<ssize_t>(total_sent);
}

// Bounded, byte-at-a-time (same technique HttpdReq.cpp already uses for the ESP32 backend's own
// multipart parsing) since request lines/headers are short and this isn't a hot path.
bool read_line(int socket_fd, std::string& out_line, TickType_t deadline) {
    out_line.clear();
    char byte;
    while (out_line.size() < MAX_LINE_LENGTH) {
        if (xTaskGetTickCount() >= deadline) {
            return false; // Connection exceeded its total budget.
        }
        ssize_t received = receive_retry(socket_fd, &byte, 1);
        if (received <= 0) {
            return false;
        }
        if (byte == '\n') {
            if (!out_line.empty() && out_line.back() == '\r') {
                out_line.pop_back();
            }
            return true;
        }
        out_line.push_back(byte);
    }
    return false; // Line exceeded MAX_LINE_LENGTH without a terminator.
}

} // namespace

struct HttpServerRequest {
    int socket_fd;
    HttpMethod method;
    std::string path; // e.g. "/fs/list"; not including the query string
    std::string query; // still URL-encoded, without the leading '?'
    std::vector<std::pair<std::string, std::string>> headers;
    uint64_t content_length = 0;
    uint64_t body_remaining = 0;

    status_code_t status_code = 200;
    std::string content_type = "text/plain";
    std::vector<std::pair<std::string, std::string>> extra_headers;
    bool response_sent = false;
    bool chunked = false;
};

struct HttpServer {
    std::string address;
    uint16_t configured_port = 0;
    uint32_t stack_size = DEFAULT_STACK_SIZE;
    std::vector<HttpServerRequestHandler> handlers;

    Mutex mutex {};
    int listen_fd = -1;
    uint16_t bound_port = 0;
    volatile bool stop_requested = false;
    volatile bool running = false;
    SemaphoreHandle_t stopped_semaphore = nullptr;

    HttpServer() { mutex_construct(&mutex); }
    ~HttpServer() { mutex_destruct(&mutex); }
};

namespace {

// A caller-supplied header name/value/content-type reaching here unfiltered (e.g. a decoded
// upload filename) could otherwise inject extra header lines or split the response.
bool contains_crlf(const char* text) {
    return strpbrk(text, "\r\n") != nullptr;
}

const std::pair<std::string, std::string>* find_header(const HttpServerRequest* request, const char* name) {
    for (const auto& header : request->headers) {
        if (strcasecmp(header.first.c_str(), name) == 0) {
            return &header;
        }
    }
    return nullptr;
}

// Status line, Content-Type, extra headers, then either Content-Length or (if @a chunked)
// Transfer-Encoding: chunked, terminated by the blank line that starts the body.
std::string build_response_prologue(HttpServerRequest* request, bool chunked, size_t content_length) {
    const char* status_code_text;
    if (status_code_to_string(request->status_code, &status_code_text) != ERROR_NONE) status_code_text = "Unknown";
    std::string result = "HTTP/1.1 " + std::to_string(request->status_code) + " " + status_code_text + "\r\n";
    result += "Content-Type: " + request->content_type + "\r\n";
    if (chunked) {
        result += "Transfer-Encoding: chunked\r\n";
    } else {
        result += "Content-Length: " + std::to_string(content_length) + "\r\n";
    }
    for (const auto& header : request->extra_headers) {
        result += header.first + ": " + header.second + "\r\n";
    }
    result += "Connection: close\r\n\r\n";
    return result;
}

// Reads the request line + headers off `client_fd`, dispatches to the matching handler (or a
// built-in 404/400), and guarantees a response is sent even if the handler didn't send one.
void handle_connection(HttpServer* server, int client_fd) {
    timeval receive_timeout {
        .tv_sec = CONNECTION_RECEIVE_TIMEOUT_MS / 1000,
        .tv_usec = (CONNECTION_RECEIVE_TIMEOUT_MS % 1000) * 1000,
    };
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &receive_timeout, sizeof(receive_timeout));

    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(CONNECTION_TOTAL_TIMEOUT_MS);

    std::string request_line;
    if (!read_line(client_fd, request_line, deadline)) {
        return; // Malformed/empty request, or the connection's total budget ran out.
    }

    size_t first_space = request_line.find(' ');
    size_t second_space = request_line.find(' ', first_space == std::string::npos ? std::string::npos : first_space + 1);
    if (first_space == std::string::npos || second_space == std::string::npos) {
        return;
    }

    HttpServerRequest request {};
    request.socket_fd = client_fd;
    auto method_text = request_line.substr(0, first_space);
    if (http_method_from_string(method_text.c_str(), &request.method) != ERROR_NONE) {
        return;
    }

    std::string target = request_line.substr(first_space + 1, second_space - first_space - 1);
    size_t query_start = target.find('?');
    request.path = query_start == std::string::npos ? target : target.substr(0, query_start);
    if (query_start != std::string::npos) {
        request.query = target.substr(query_start + 1);
    }
    const std::string& path = request.path;

    std::string header_line;
    while (read_line(client_fd, header_line, deadline) && !header_line.empty()) {
        if (request.headers.size() >= MAX_HEADER_COUNT) {
            continue; // Cap reached: known callers only need the first handful (Content-Type etc).
        }
        size_t colon = header_line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        std::string name = header_line.substr(0, colon);
        size_t value_start = header_line.find_first_not_of(' ', colon + 1);
        std::string value = value_start == std::string::npos ? "" : header_line.substr(value_start);
        request.headers.emplace_back(std::move(name), std::move(value));
    }

    if (const auto* content_length_header = find_header(&request, "Content-Length")) {
        const std::string& raw = content_length_header->second;
        errno = 0;
        char* end = nullptr;
        unsigned long long parsed = strtoull(raw.c_str(), &end, 10);
        bool all_digits = !raw.empty() && raw.find_first_not_of("0123456789") == std::string::npos;
        if (!all_digits || errno == ERANGE || end != raw.c_str() + raw.size()) {
            http_server_request_send_error(&request, 400, "Invalid Content-Length");
            return;
        }
        request.content_length = parsed;
        request.body_remaining = parsed;
    }

    const HttpServerRequestHandler* matched = nullptr;
    for (const auto& handler : server->handlers) {
        if (handler.method != request.method) {
            continue;
        }
        size_t handler_uri_length = strlen(handler.uri);
        bool is_wildcard = handler_uri_length > 0 && handler.uri[handler_uri_length - 1] == '*';
        bool matches = is_wildcard
            ? path.compare(0, handler_uri_length - 1, handler.uri, handler_uri_length - 1) == 0
            : path == handler.uri;
        if (matches) {
            matched = &handler;
            break;
        }
    }

    if (matched == nullptr) {
        http_server_request_send_error(&request, 404, "Not Found");
        return;
    }

    matched->callback(&request, matched->user_ctx);

    if (!request.response_sent) {
        LOG_W(TAG, "Handler for %s did not send a response", matched->uri);
        http_server_request_send_error(&request, 500, "Handler did not send a response");
    }
}

void server_task_main(void* raw_server) {
    auto* server = static_cast<HttpServer*>(raw_server);
    LOG_I(TAG, "Listening on port %u", static_cast<unsigned>(server->bound_port));

    while (!server->stop_requested) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server->listen_fd, &read_fds);
        timeval timeout {
            .tv_sec = 0,
            .tv_usec = ACCEPT_POLL_TIMEOUT_MS * 1000,
        };

        int ready = select(server->listen_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        if (ready <= 0) {
            continue; // Timeout or interrupted: loop back around to re-check stop_requested.
        }

        int client_fd = accept(server->listen_fd, nullptr, nullptr);
        if (client_fd < 0) {
            continue;
        }
        handle_connection(server, client_fd);
        close(client_fd);
    }

    xSemaphoreGive(server->stopped_semaphore);
    vTaskDelete(nullptr);
}

} // namespace

extern "C" {

HttpServer* http_server_alloc(const HttpServerConfig* config) {
    auto* server = new (std::nothrow) HttpServer();
    if (server == nullptr) {
        return nullptr;
    }

    server->address = config->address != nullptr ? config->address : "0.0.0.0";
    server->configured_port = config->port;
    server->stack_size = config->stack_size != 0 ? config->stack_size : DEFAULT_STACK_SIZE;
    server->handlers.assign(config->handlers, config->handlers + config->handler_count);
    return server;
}

void http_server_free(HttpServer* server) {
    if (server == nullptr) {
        return;
    }
    http_server_stop(server);
    delete server;
}

error_t http_server_start(HttpServer* server) {
    mutex_lock(&server->mutex);
    if (server->running) {
        mutex_unlock(&server->mutex);
        return ERROR_NONE;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        mutex_unlock(&server->mutex);
        return ERROR_RESOURCE;
    }

    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(server->configured_port);
    if (inet_pton(AF_INET, server->address.c_str(), &address.sin_addr) != 1) {
        address.sin_addr.s_addr = INADDR_ANY;
    }

    if (bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 ||
        listen(fd, LISTEN_BACKLOG) != 0) {
        LOG_E(TAG, "Failed to bind/listen on port %u", static_cast<unsigned>(server->configured_port));
        close(fd);
        mutex_unlock(&server->mutex);
        return ERROR_RESOURCE;
    }

    socklen_t address_length = sizeof(address);
    getsockname(fd, reinterpret_cast<sockaddr*>(&address), &address_length);

    server->listen_fd = fd;
    server->bound_port = ntohs(address.sin_port);
    server->stop_requested = false;
    server->stopped_semaphore = xSemaphoreCreateBinary();
    if (server->stopped_semaphore == nullptr) {
        close(fd);
        server->listen_fd = -1;
        mutex_unlock(&server->mutex);
        return ERROR_RESOURCE;
    }

    TaskHandle_t task_handle = nullptr;
    if (xTaskCreate(server_task_main, "http-server", server->stack_size / sizeof(StackType_t), server, tskIDLE_PRIORITY + 1, &task_handle) != pdPASS) {
        close(fd);
        server->listen_fd = -1;
        vSemaphoreDelete(server->stopped_semaphore);
        server->stopped_semaphore = nullptr;
        mutex_unlock(&server->mutex);
        return ERROR_RESOURCE;
    }

    server->running = true;
    mutex_unlock(&server->mutex);
    return ERROR_NONE;
}

void http_server_stop(HttpServer* server) {
    mutex_lock(&server->mutex);
    if (!server->running) {
        mutex_unlock(&server->mutex);
        return;
    }
    server->stop_requested = true;
    SemaphoreHandle_t semaphore = server->stopped_semaphore;
    mutex_unlock(&server->mutex);

    // Not held while waiting: the task itself never touches `mutex`, so this is only about not
    // blocking a concurrent http_server_is_started()/get_port() call for the whole shutdown.
    xSemaphoreTake(semaphore, portMAX_DELAY);

    mutex_lock(&server->mutex);
    close(server->listen_fd);
    server->listen_fd = -1;
    vSemaphoreDelete(server->stopped_semaphore);
    server->stopped_semaphore = nullptr;
    server->running = false;
    mutex_unlock(&server->mutex);
}

bool http_server_is_started(HttpServer* server) {
    mutex_lock(&server->mutex);
    bool started = server->running;
    mutex_unlock(&server->mutex);
    return started;
}

uint16_t http_server_get_port(HttpServer* server) {
    mutex_lock(&server->mutex);
    uint16_t port = server->running ? server->bound_port : 0;
    mutex_unlock(&server->mutex);
    return port;
}

HttpMethod http_server_request_get_method(HttpServerRequest* request) {
    return request->method;
}

size_t http_server_request_get_uri(HttpServerRequest* request, char* buffer, size_t buffer_size) {
    if (buffer_size > 0) {
        snprintf(buffer, buffer_size, "%s", request->path.c_str());
    }
    return request->path.size();
}

size_t http_server_request_get_query(HttpServerRequest* request, char* buffer, size_t buffer_size) {
    if (buffer_size > 0) {
        snprintf(buffer, buffer_size, "%s", request->query.c_str());
    }
    return request->query.size();
}

size_t http_server_request_get_header(HttpServerRequest* request, const char* name, char* buffer, size_t buffer_size) {
    const auto* header = find_header(request, name);
    if (header == nullptr) {
        return 0;
    }
    if (buffer_size > 0) {
        snprintf(buffer, buffer_size, "%s", header->second.c_str());
    }
    return header->second.size();
}

uint64_t http_server_request_get_content_length(HttpServerRequest* request) {
    return request->content_length;
}

int http_server_request_receive(HttpServerRequest* request, void* buffer, size_t buffer_size) {
    if (request->body_remaining == 0) {
        return 0; // End of the declared body.
    }
    if (buffer_size > request->body_remaining) {
        buffer_size = static_cast<size_t>(request->body_remaining);
    }
    ssize_t received = receive_retry(request->socket_fd, buffer, buffer_size);
    if (received > 0) {
        request->body_remaining -= static_cast<uint64_t>(received);
    }
    return static_cast<int>(received);
}

void http_server_request_set_status(HttpServerRequest* request, status_code_t status_code) {
    if (!request->response_sent) {
        request->status_code = status_code;
    }
}

void http_server_request_set_content_type(HttpServerRequest* request, const char* content_type) {
    if (request->response_sent) {
        return;
    }
    if (contains_crlf(content_type)) {
        LOG_W(TAG, "Rejected content type containing CR/LF");
        return;
    }
    request->content_type = content_type;
}

void http_server_request_set_header(HttpServerRequest* request, const char* name, const char* value) {
    if (request->response_sent) {
        return;
    }
    if (contains_crlf(name) || contains_crlf(value)) {
        LOG_W(TAG, "Rejected header containing CR/LF: %s", name);
        return;
    }
    request->extra_headers.emplace_back(name, value);
}

error_t http_server_request_send(HttpServerRequest* request, const void* data, size_t length) {
    if (request->response_sent) {
        return ERROR_INVALID_STATE;
    }
    request->response_sent = true;

    std::string prologue = build_response_prologue(request, false, length);
    if (send_retry(request->socket_fd, prologue.data(), prologue.size()) < 0) {
        return ERROR_RESOURCE;
    }
    if (length > 0) {
        if (send_retry(request->socket_fd, data, length) < 0) {
            return ERROR_RESOURCE;
        }
    }
    return ERROR_NONE;
}

error_t http_server_request_send_string(HttpServerRequest* request, const char* text) {
    return http_server_request_send(request, text, strlen(text));
}

error_t http_server_request_send_error(HttpServerRequest* request, int status_code, const char* message) {
    http_server_request_set_status(request, status_code);
    return http_server_request_send_string(request, message);
}

error_t http_server_request_send_chunk_start(HttpServerRequest* request) {
    if (request->response_sent) {
        return ERROR_INVALID_STATE;
    }
    request->response_sent = true;
    request->chunked = true;

    std::string prologue = build_response_prologue(request, true, 0);
    if (send_retry(request->socket_fd, prologue.data(), prologue.size()) < 0) {
        return ERROR_RESOURCE;
    }
    return ERROR_NONE;
}

error_t http_server_request_send_chunk(HttpServerRequest* request, const void* data, size_t length) {
    if (!request->chunked) {
        return ERROR_INVALID_STATE;
    }
    if (length == 0) {
        return ERROR_NONE; // A zero-length chunk is indistinguishable from the terminator.
    }

    char size_line[32];
    int size_line_length = snprintf(size_line, sizeof(size_line), "%zx\r\n", length);
    if (send_retry(request->socket_fd, size_line, static_cast<size_t>(size_line_length)) < 0) {
        return ERROR_RESOURCE;
    }
    if (send_retry(request->socket_fd, data, length) < 0) {
        return ERROR_RESOURCE;
    }
    if (send_retry(request->socket_fd, "\r\n", 2) < 0) {
        return ERROR_RESOURCE;
    }
    return ERROR_NONE;
}

error_t http_server_request_send_chunk_end(HttpServerRequest* request) {
    if (!request->chunked) {
        return ERROR_INVALID_STATE;
    }
    request->chunked = false; // A second call now no-ops instead of re-sending the terminator.
    if (send_retry(request->socket_fd, "0\r\n\r\n", 5) < 0) {
        return ERROR_RESOURCE;
    }
    return ERROR_NONE;
}

} // extern "C"
