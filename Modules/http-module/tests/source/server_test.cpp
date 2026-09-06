#include "doctest.h"

#include <http/server.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <string>

namespace {

// Connects to 127.0.0.1:port, sends `request` verbatim, and returns whatever the server sent
// back before closing the connection.
std::string send_request(uint16_t port, const std::string& request) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(fd >= 0);

    struct sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

    // The FreeRTOS POSIX port's tick signal can interrupt a blocking syscall on this thread, so
    // every blocking call below retries on EINTR (matches server_posix.cpp's own recv_retry()).
    int connect_result;
    do {
        connect_result = connect(fd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address));
    } while (connect_result != 0 && errno == EINTR);
    REQUIRE(connect_result == 0);

    // Without this, a server bug that leaves the connection open makes this block in recv()
    // until CTest's/CI's external job timeout, instead of failing this test.
    struct timeval receive_timeout { .tv_sec = 5, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &receive_timeout, sizeof(receive_timeout));

    size_t total_sent = 0;
    while (total_sent < request.size()) {
        ssize_t sent = send(fd, request.data() + total_sent, request.size() - total_sent, 0);
        if (sent < 0) {
            REQUIRE(errno == EINTR);
            continue;
        }
        total_sent += static_cast<size_t>(sent);
    }

    std::string response;
    char chunk[256];
    ssize_t received;
    while (true) {
        received = recv(fd, chunk, sizeof(chunk), 0);
        if (received < 0 && errno == EINTR) {
            continue;
        }
        if (received <= 0) {
            break;
        }
        response.append(chunk, static_cast<size_t>(received));
    }
    close(fd);
    return response;
}

error_t handle_ping(struct HttpServerRequest* request, void*) {
    return http_server_request_send_string(request, "pong");
}

error_t handle_wildcard(struct HttpServerRequest* request, void*) {
    char uri[64] {};
    http_server_request_get_uri(request, uri, sizeof(uri));
    return http_server_request_send_string(request, uri);
}

error_t handle_redirect(struct HttpServerRequest* request, void*) {
    http_server_request_set_status(request, 302);
    http_server_request_set_header(request, "Location", "/elsewhere");
    return http_server_request_send(request, nullptr, 0);
}

error_t handle_chunked(struct HttpServerRequest* request, void*) {
    if (http_server_request_send_chunk_start(request) != ERROR_NONE) {
        return ERROR_UNDEFINED;
    }
    http_server_request_send_chunk(request, "one-", 4);
    http_server_request_send_chunk(request, "two", 3);
    http_server_request_send_chunk_end(request);
    return ERROR_NONE;
}

error_t handle_echo(struct HttpServerRequest* request, void*) {
    char query[64] {};
    http_server_request_get_query(request, query, sizeof(query));

    char body[64] {};
    size_t total_read = 0;
    uint64_t content_length = http_server_request_get_content_length(request);
    while (total_read < content_length && total_read < sizeof(body) - 1) {
        int read = http_server_request_receive(request, body + total_read, sizeof(body) - 1 - total_read);
        if (read <= 0) {
            break;
        }
        total_read += static_cast<size_t>(read);
    }

    std::string response = std::string("query=") + query + " body=" + body;
    return http_server_request_send_string(request, response.c_str());
}

} // namespace

TEST_CASE("http_server_start binds an OS-assigned port and serves a registered handler") {
    HttpServerRequestHandler handlers[] = {
        { .uri = "/ping", .method = HTTP_METHOD_GET, .callback = handle_ping, .user_ctx = nullptr },
    };
    HttpServerConfig config { .port = 0, .address = "0.0.0.0", .stack_size = 0, .handlers = handlers, .handler_count = 1 };

    HttpServer* server = http_server_alloc(&config);
    REQUIRE(server != nullptr);
    CHECK_EQ(http_server_start(server), ERROR_NONE);
    CHECK(http_server_is_started(server));

    uint16_t port = http_server_get_port(server);
    CHECK_NE(port, 0);

    std::string response = send_request(port, "GET /ping HTTP/1.1\r\nHost: x\r\n\r\n");
    CHECK_NE(response.find("200"), std::string::npos);
    CHECK_NE(response.find("pong"), std::string::npos);

    http_server_free(server);
}

TEST_CASE("http_server_start serves query string and request body to the handler") {
    HttpServerRequestHandler handlers[] = {
        { .uri = "/echo", .method = HTTP_METHOD_PUT, .callback = handle_echo, .user_ctx = nullptr },
    };
    HttpServerConfig config { .port = 0, .address = "0.0.0.0", .stack_size = 0, .handlers = handlers, .handler_count = 1 };

    HttpServer* server = http_server_alloc(&config);
    REQUIRE(server != nullptr);
    REQUIRE_EQ(http_server_start(server), ERROR_NONE);
    uint16_t port = http_server_get_port(server);

    std::string body = "hello";
    std::string request = "PUT /echo?name=world HTTP/1.1\r\nHost: x\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
    std::string response = send_request(port, request);
    CHECK_NE(response.find("query=name=world"), std::string::npos);
    CHECK_NE(response.find("body=hello"), std::string::npos);

    http_server_free(server);
}

TEST_CASE("an unmatched uri gets a 404") {
    HttpServerRequestHandler handlers[] = {
        { .uri = "/ping", .method = HTTP_METHOD_GET, .callback = handle_ping, .user_ctx = nullptr },
    };
    HttpServerConfig config { .port = 0, .address = "0.0.0.0", .stack_size = 0, .handlers = handlers, .handler_count = 1 };

    HttpServer* server = http_server_alloc(&config);
    REQUIRE(server != nullptr);
    REQUIRE_EQ(http_server_start(server), ERROR_NONE);
    uint16_t port = http_server_get_port(server);

    std::string response = send_request(port, "GET /missing HTTP/1.1\r\nHost: x\r\n\r\n");
    CHECK_NE(response.find("404"), std::string::npos);

    http_server_free(server);
}

TEST_CASE("a trailing '*' route matches by prefix and get_uri returns the matched path") {
    HttpServerRequestHandler handlers[] = {
        { .uri = "/fs/*", .method = HTTP_METHOD_GET, .callback = handle_wildcard, .user_ctx = nullptr },
    };
    HttpServerConfig config { .port = 0, .address = "0.0.0.0", .stack_size = 0, .handlers = handlers, .handler_count = 1 };

    HttpServer* server = http_server_alloc(&config);
    REQUIRE(server != nullptr);
    REQUIRE_EQ(http_server_start(server), ERROR_NONE);
    uint16_t port = http_server_get_port(server);

    std::string response = send_request(port, "GET /fs/list?path=/data HTTP/1.1\r\nHost: x\r\n\r\n");
    CHECK_NE(response.find("200"), std::string::npos);
    CHECK_NE(response.find("/fs/list"), std::string::npos);
    // get_uri() must not include the query string.
    CHECK_EQ(response.find("path=/data"), std::string::npos);

    http_server_free(server);
}

TEST_CASE("set_status and set_header apply to the response") {
    HttpServerRequestHandler handlers[] = {
        { .uri = "/redirect", .method = HTTP_METHOD_GET, .callback = handle_redirect, .user_ctx = nullptr },
    };
    HttpServerConfig config { .port = 0, .address = "0.0.0.0", .stack_size = 0, .handlers = handlers, .handler_count = 1 };

    HttpServer* server = http_server_alloc(&config);
    REQUIRE(server != nullptr);
    REQUIRE_EQ(http_server_start(server), ERROR_NONE);
    uint16_t port = http_server_get_port(server);

    std::string response = send_request(port, "GET /redirect HTTP/1.1\r\nHost: x\r\n\r\n");
    CHECK_NE(response.find("302"), std::string::npos);
    CHECK_NE(response.find("Location: /elsewhere"), std::string::npos);

    http_server_free(server);
}

TEST_CASE("a chunked response delivers all chunks concatenated") {
    HttpServerRequestHandler handlers[] = {
        { .uri = "/chunked", .method = HTTP_METHOD_GET, .callback = handle_chunked, .user_ctx = nullptr },
    };
    HttpServerConfig config { .port = 0, .address = "0.0.0.0", .stack_size = 0, .handlers = handlers, .handler_count = 1 };

    HttpServer* server = http_server_alloc(&config);
    REQUIRE(server != nullptr);
    REQUIRE_EQ(http_server_start(server), ERROR_NONE);
    uint16_t port = http_server_get_port(server);

    std::string response = send_request(port, "GET /chunked HTTP/1.1\r\nHost: x\r\n\r\n");
    CHECK_NE(response.find("Transfer-Encoding: chunked"), std::string::npos);
    // Wire format is "<hex-len>\r\n<data>\r\n" per chunk, terminated by "0\r\n\r\n". The two
    // chunks are not contiguous in the raw response, so check each piece and the framing.
    CHECK_NE(response.find("4\r\none-\r\n"), std::string::npos);
    CHECK_NE(response.find("3\r\ntwo\r\n"), std::string::npos);
    CHECK(response.ends_with("0\r\n\r\n"));

    http_server_free(server);
}

TEST_CASE("http_server_stop closes the listening port") {
    HttpServerConfig config { .port = 0, .address = "0.0.0.0", .stack_size = 0, .handlers = nullptr, .handler_count = 0 };

    HttpServer* server = http_server_alloc(&config);
    REQUIRE(server != nullptr);
    REQUIRE_EQ(http_server_start(server), ERROR_NONE);
    uint16_t port = http_server_get_port(server);

    http_server_stop(server);
    CHECK_FALSE(http_server_is_started(server));

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    CHECK_NE(connect(fd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)), 0);
    close(fd);

    http_server_free(server);
}
