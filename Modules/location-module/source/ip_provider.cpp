// SPDX-License-Identifier: Apache-2.0
#include <location/ip_provider.h>

#include <tactility/concurrent/task_event_group.h>
#include <tactility/concurrent/thread.h>
#include <tactility/log.h>
#include <tactility/system_event.h>
#include <tactility/time.h>

#include <TactilityCpp/Memory.h>
#include <TactilityCpp/std/string.h>

#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <strings.h>

#define TAG "location_ip"

namespace {

// The free ip-api endpoint only supports plain HTTP
constexpr auto* API_HOST = "ip-api.com";
constexpr auto* API_PATH = "/line/?fields=status,message,lat,lon";
constexpr uint64_t RETRY_INTERVAL_MS = 60 * 1000;
// Bounds the request rate when the network connects repeatedly
constexpr uint64_t MIN_INTERVAL_MS = 10 * 1000;
// Wake up interval to check whether the fallback became needed
constexpr uint32_t CHECK_INTERVAL_MS = 10 * 1000;
constexpr int SOCKET_TIMEOUT_SECONDS = 5;
constexpr size_t RESPONSE_MAX_LENGTH = 1024;
// IP geolocation is accurate to roughly city level
constexpr float ACCURACY_METERS = 25000.0f;

/** A lookup result */
struct CachedLookup {
    // The network of the lookup, see trackedNetwork
    uint64_t network = 0;
    double latitude = NAN;
    double longitude = NAN;
};

struct IpProvider {
    LocationHost* host = nullptr;
    Thread* thread = nullptr;
    std::atomic<bool> running = false;
    TaskEventGroup group {};
    uint32_t stopBit = 0;
    SystemEventSubscription networkSubscription {};
};

// The last lookup result, kept across provider runs until the network isn't tracked anymore.
// Only accessed by the provider thread and by ip_provider_untrack_network() after the provider stopped.
CachedLookup* cache = nullptr;

// The local IP address and gateway of the last connected network, 0 when unknown.
// A disconnect keeps the last known network, so reconnecting to it doesn't need a new lookup.
std::atomic<uint64_t> trackedNetwork = 0;

void onNetworkConnected(SystemEvent* event, void*) {
    NetworkConnectedEvent connected;
    if (event->data_len < sizeof(connected)) {
        return;
    }
    memcpy(&connected, event->data, sizeof(connected));
    trackedNetwork = (static_cast<uint64_t>(connected.ipv4_addr) << 32) | connected.gateway;
}

uint64_t getUptimeMs() {
    return get_micros_since_boot() / 1000;
}

// The FreeRTOS POSIX port's tick signal can interrupt blocking socket calls, so they retry on EINTR

bool connectWithTimeout(int fd, const sockaddr* address, socklen_t addressLength) {
    const int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    int result = connect(fd, address, addressLength);
    if (result < 0 && errno == EINPROGRESS) {
        fd_set writeSet;
        do {
            FD_ZERO(&writeSet);
            FD_SET(fd, &writeSet);
            timeval timeout { .tv_sec = SOCKET_TIMEOUT_SECONDS, .tv_usec = 0 };
            result = select(fd + 1, nullptr, &writeSet, nullptr, &timeout);
        } while (result < 0 && errno == EINTR);
        int error = 0;
        socklen_t errorLength = sizeof(error);
        if (result == 1 && getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &errorLength) == 0 && error == 0) {
            result = 0;
        } else {
            result = -1;
        }
    }
    fcntl(fd, F_SETFL, flags);
    return result == 0;
}

bool sendAll(int fd, const char* data, size_t length) {
    size_t sent = 0;
    while (sent < length) {
        const ssize_t result = send(fd, data + sent, length - sent, 0);
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        sent += static_cast<size_t>(result);
    }
    return true;
}

/** Receive until the server closes the connection */
bool receiveAll(int fd, char* buffer, size_t bufferSize) {
    size_t received = 0;
    while (received < bufferSize - 1) {
        const ssize_t result = recv(fd, buffer + received, bufferSize - 1 - received, 0);
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (result == 0) {
            break;
        }
        received += static_cast<size_t>(result);
    }
    buffer[received] = '\0';
    return received > 0;
}

bool httpGet(const char* host, const char* path, char* response, size_t responseSize) {
    addrinfo hints {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* addresses = nullptr;
    if (getaddrinfo(host, "80", &hints, &addresses) != 0 || addresses == nullptr) {
        return false;
    }

    const int fd = socket(addresses->ai_family, addresses->ai_socktype, addresses->ai_protocol);
    if (fd < 0) {
        freeaddrinfo(addresses);
        return false;
    }

    timeval timeout { .tv_sec = SOCKET_TIMEOUT_SECONDS, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    char request[160];
    const int requestLength = snprintf(request, sizeof(request), "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);

    const bool success = connectWithTimeout(fd, addresses->ai_addr, addresses->ai_addrlen) &&
        sendAll(fd, request, static_cast<size_t>(requestLength)) &&
        receiveAll(fd, response, responseSize);

    close(fd);
    freeaddrinfo(addresses);
    return success;
}

void report(IpProvider& provider, double latitude, double longitude) {
    const Location location {
        .latitude = latitude,
        .longitude = longitude,
        .horizontal_accuracy = ACCURACY_METERS,
        .altitude = NAN,
        .ellipsoidal_altitude = NAN,
        .vertical_accuracy = NAN,
        .speed = NAN,
        .speed_accuracy = NAN,
        .course = NAN,
        .course_accuracy = NAN,
        .satellites = 0,
        .time_ms = 0,
        .uptime_ms = 0,
        .provider = {},
    };
    location_host_report(provider.host, &location);
}

/** @return the delay until the next lookup is allowed */
uint64_t lookup(IpProvider& provider, uint64_t network) {
    tt::std::string response(RESPONSE_MAX_LENGTH, '\0');
    if (!httpGet(API_HOST, API_PATH, response.data(), response.size())) {
        LOG_W(TAG, "Request failed");
        return RETRY_INTERVAL_MS;
    }

    IpApiResponse parsed;
    if (!ip_api_parse_response(response.c_str(), &parsed)) {
        LOG_W(TAG, "Malformed response");
        return RETRY_INTERVAL_MS;
    }

    uint64_t delay = RETRY_INTERVAL_MS;
    if (parsed.status_code != 200) {
        LOG_W(TAG, "HTTP status %d", parsed.status_code);
    } else if (!parsed.success) {
        LOG_W(TAG, "Lookup failed: %s", parsed.message);
    } else {
        if (cache == nullptr) {
            cache = tt::newObject<CachedLookup>();
        }
        if (cache != nullptr) {
            *cache = { .network = network, .latitude = parsed.latitude, .longitude = parsed.longitude };
        }
        report(provider, parsed.latitude, parsed.longitude);
        LOG_D(TAG, "Lookup succeeded");
        delay = MIN_INTERVAL_MS;
    }

    if (parsed.rate_remaining == 0 && parsed.rate_reset_seconds > 0) {
        delay = std::max(delay, static_cast<uint64_t>(parsed.rate_reset_seconds) * 1000);
    }
    return delay;
}

int32_t threadMain(void* context) {
    auto& provider = *static_cast<IpProvider*>(context);
    uint64_t lastAttemptMs = 0;
    uint64_t nextAttemptMs = 0;
    bool attempted = false;
    // Whether the cached result was reported during this run
    bool reportedCache = false;

    while (provider.running) {
        const uint64_t now = getUptimeMs();
        const uint64_t waitMs = std::min<uint64_t>(CHECK_INTERVAL_MS, nextAttemptMs > now ? nextAttemptMs - now : 0);
        uint32_t flags = 0;
        task_event_group_wait_any(&provider.group, &flags, millis_to_ticks(static_cast<uint32_t>(waitMs)));
        if (!provider.running) {
            break;
        }

        // The public IP address can change with the network
        if (system_event_poll(&provider.networkSubscription) == ERROR_NONE && attempted) {
            nextAttemptMs = lastAttemptMs + MIN_INTERVAL_MS;
        }

        if (!location_host_needs_fallback()) {
            continue;
        }

        const uint64_t network = trackedNetwork;
        if (cache != nullptr && cache->network == network) {
            if (!reportedCache) {
                report(provider, cache->latitude, cache->longitude);
                reportedCache = true;
            }
            continue;
        }

        const uint64_t attemptMs = getUptimeMs();
        if (attemptMs < nextAttemptMs) {
            continue;
        }
        attempted = true;
        lastAttemptMs = attemptMs;
        nextAttemptMs = attemptMs + lookup(provider, network);
        reportedCache = cache != nullptr && cache->network == network;
    }
    return 0;
}

void* create(LocationHost* host) {
    auto* provider = tt::newObject<IpProvider>();
    if (provider == nullptr) {
        return nullptr;
    }
    provider->host = host;
    provider->running = true;
    task_event_group_construct(&provider->group);
    if (task_event_group_claim_bit(&provider->group, &provider->stopBit) != ERROR_NONE) {
        task_event_group_destruct(&provider->group);
        tt::deleteObject(provider);
        return nullptr;
    }
    provider->networkSubscription.event.type = KERNEL_EVENT_NETWORK_CONNECTED;
    if (system_event_subscribe(&provider->networkSubscription, &provider->group) != ERROR_NONE) {
        task_event_group_destruct(&provider->group);
        tt::deleteObject(provider);
        return nullptr;
    }

    provider->thread = thread_alloc_full("location_ip", 4096, threadMain, provider, -1);
    if (provider->thread != nullptr) {
        // Settings are loaded before providers start, so the thread doesn't access flash
        thread_set_stack_external(provider->thread, true);
    }
    if (provider->thread == nullptr || thread_start(provider->thread) != ERROR_NONE) {
        if (provider->thread != nullptr) {
            thread_free(provider->thread);
        }
        system_event_unsubscribe(&provider->networkSubscription);
        task_event_group_destruct(&provider->group);
        tt::deleteObject(provider);
        return nullptr;
    }
    return provider;
}

void requestStop(void* context) {
    auto* provider = static_cast<IpProvider*>(context);
    provider->running = false;
    task_event_group_signal(&provider->group, provider->stopBit);
}

bool waitStopped(void* context, TickType_t timeout) {
    auto* provider = static_cast<IpProvider*>(context);
    if (thread_join(provider->thread, timeout, millis_to_ticks(10)) != ERROR_NONE) {
        return false;
    }
    thread_free(provider->thread);
    system_event_unsubscribe(&provider->networkSubscription);
    task_event_group_destruct(&provider->group);
    tt::deleteObject(provider);
    return true;
}

std::string_view nextLine(std::string_view& text) {
    const size_t end = text.find('\n');
    std::string_view line = text.substr(0, end);
    text = end == std::string_view::npos ? std::string_view {} : text.substr(end + 1);
    if (!line.empty() && line.back() == '\r') {
        line.remove_suffix(1);
    }
    return line;
}

int parseHeaderNumber(std::string_view line, const char* name) {
    const size_t nameLength = strlen(name);
    if (line.size() <= nameLength || line[nameLength] != ':' || strncasecmp(line.data(), name, nameLength) != 0) {
        return -1;
    }
    return atoi(tt::std::string(line.substr(nameLength + 1)).c_str());
}

} // namespace

error_t ip_provider_track_network(void) {
    return system_event_callback_add(KERNEL_EVENT_NETWORK_CONNECTED, onNetworkConnected, nullptr);
}

void ip_provider_untrack_network(void) {
    system_event_callback_remove(KERNEL_EVENT_NETWORK_CONNECTED, onNetworkConnected);
    if (cache != nullptr) {
        tt::deleteObject(cache);
        cache = nullptr;
    }
}

bool ip_api_parse_response(const char* response, IpApiResponse* out) {
    *out = IpApiResponse {
        .status_code = 0,
        .rate_remaining = -1,
        .rate_reset_seconds = -1,
        .success = false,
        .message = {},
        .latitude = NAN,
        .longitude = NAN,
    };

    std::string_view text = response;
    const std::string_view statusLine = nextLine(text);
    if (!statusLine.starts_with("HTTP/1.")) {
        return false;
    }
    const size_t space = statusLine.find(' ');
    if (space == std::string_view::npos) {
        return false;
    }
    out->status_code = atoi(tt::std::string(statusLine.substr(space + 1)).c_str());

    while (true) {
        if (text.empty()) {
            return false;
        }
        const std::string_view header = nextLine(text);
        if (header.empty()) {
            break;
        }
        const int remaining = parseHeaderNumber(header, "X-Rl");
        if (remaining >= 0) {
            out->rate_remaining = remaining;
        }
        const int resetSeconds = parseHeaderNumber(header, "X-Ttl");
        if (resetSeconds >= 0) {
            out->rate_reset_seconds = resetSeconds;
        }
    }

    if (out->status_code != 200) {
        return true;
    }

    // Fields are returned one per line, in the order of the API documentation
    const std::string_view status = nextLine(text);
    if (status == "success") {
        const tt::std::string latitude(nextLine(text));
        const tt::std::string longitude(nextLine(text));
        char* latitudeEnd;
        char* longitudeEnd;
        out->latitude = strtod(latitude.c_str(), &latitudeEnd);
        out->longitude = strtod(longitude.c_str(), &longitudeEnd);
        if (latitude.empty() || *latitudeEnd != '\0' || longitude.empty() || *longitudeEnd != '\0' ||
            !std::isfinite(out->latitude) || !std::isfinite(out->longitude) ||
            std::fabs(out->latitude) > 90.0 || std::fabs(out->longitude) > 180.0) {
            return false;
        }
        out->success = true;
    } else if (status == "fail") {
        const std::string_view message = nextLine(text).substr(0, IP_API_MESSAGE_MAX_LENGTH);
        memcpy(out->message, message.data(), message.size());
        out->message[message.size()] = '\0';
    } else {
        return false;
    }
    return true;
}

const LocationProviderApi ip_location_provider = {
    .id = "ip",
    .fallback = true,
    .create = create,
    .request_stop = requestStop,
    .wait_stopped = waitStopped,
};
