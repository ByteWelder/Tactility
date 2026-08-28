// SPDX-License-Identifier: Apache-2.0
#include <http/private/download.h>

#include <esp_http_client.h>

#include <tactility/freertos/task.h>

namespace {

// RAII: guarantees esp_http_client_close()/_cleanup() run on every exit path below.
class EspDownloadClient {
    esp_http_client_handle_t client = nullptr;
    bool isOpen = false;

public:
    ~EspDownloadClient() {
        if (isOpen) {
            esp_http_client_close(client);
        }
        if (client != nullptr) {
            esp_http_client_cleanup(client);
        }
    }

    bool init(const esp_http_client_config_t& config) {
        client = esp_http_client_init(&config);
        return client != nullptr;
    }

    bool open() {
        if (esp_http_client_open(client, 0) != ESP_OK) {
            return false;
        }
        isOpen = true;
        return true;
    }

    bool fetchHeaders() const { return esp_http_client_fetch_headers(client) >= 0; }

    int getStatusCode() const { return esp_http_client_get_status_code(client); }

    int getContentLength() const { return esp_http_client_get_content_length(client); }

    int read(char* buffer, int size) const { return esp_http_client_read(client, buffer, size); }
};

// esp_http_client_config_t::cert_pem needs a NUL-terminated buffer.
bool read_certificate(const std::string& certPath, std::string& outCertificate) {
    auto* file = fopen(certPath.c_str(), "rb");
    if (file == nullptr) {
        return false;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size <= 0) {
        fclose(file);
        return false;
    }

    outCertificate.resize(static_cast<size_t>(size));
    size_t read_bytes = fread(outCertificate.data(), 1, static_cast<size_t>(size), file);
    fclose(file);
    return read_bytes == static_cast<size_t>(size);
}

}

HttpDownloadEvent http_download_run(const std::string& url, const std::string& certPath, const std::string& targetPath, HttpDownloadLink* link) {
    if (http_download_is_cancelled(link)) {
        return http_download_make_cancelled_event();
    }

    std::string certificate;
    if (!read_certificate(certPath, certificate)) {
        return http_download_make_error_event("Failed to read certificate file");
    }

    esp_http_client_config_t config {};
    config.url = url.c_str();
    config.auth_type = HTTP_AUTH_TYPE_NONE;
    config.cert_pem = certificate.c_str();
    config.cert_len = certificate.size() + 1;
    config.tls_version = ESP_HTTP_CLIENT_TLS_VER_TLS_1_2;
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 5000;
    config.transport_type = HTTP_TRANSPORT_OVER_SSL;

    EspDownloadClient client;
    if (!client.init(config)) {
        return http_download_make_error_event("Failed to initialize HTTP client");
    }
    if (!client.open()) {
        return http_download_make_error_event("Failed to open connection");
    }
    if (!client.fetchHeaders()) {
        return http_download_make_error_event("Failed to fetch response headers");
    }

    auto status_code = client.getStatusCode();
    if (status_code < 200 || status_code >= 300) {
        return http_download_make_error_event("Server response is not OK", status_code);
    }

    if (http_download_is_cancelled(link)) {
        return http_download_make_cancelled_event(status_code);
    }

    // Downloaded to a sibling ".tmp" file first and only renamed onto targetPath on success, so a
    // failed/cancelled download (or a crash mid-write) never leaves a partial file at targetPath.
    auto tempPath = targetPath + ".tmp";
    auto* file = fopen(tempPath.c_str(), "wb");
    if (file == nullptr) {
        return http_download_make_error_event("Failed to open target file", status_code);
    }

    auto bytes_left = client.getContentLength();
    char buffer[512];
    while (bytes_left > 0) {
        if (http_download_is_cancelled(link)) {
            fclose(file);
            remove(tempPath.c_str());
            return http_download_make_cancelled_event(status_code);
        }
        int data_read = client.read(buffer, sizeof(buffer));
        if (data_read <= 0) {
            fclose(file);
            remove(tempPath.c_str());
            return http_download_make_error_event("Failed to read response data", status_code);
        }
        bytes_left -= data_read;
        if (fwrite(buffer, 1, static_cast<size_t>(data_read), file) != static_cast<size_t>(data_read)) {
            fclose(file);
            remove(tempPath.c_str());
            return http_download_make_error_event("Failed to write downloaded data", status_code);
        }
        taskYIELD();
    }
    fclose(file);

    // Some embedded filesystems (e.g. FATFS) reject rename() onto an existing path instead of
    // replacing it like POSIX does. Clear the way first.
    remove(targetPath.c_str());
    if (rename(tempPath.c_str(), targetPath.c_str()) != 0) {
        remove(tempPath.c_str());
        return http_download_make_error_event("Failed to finalize downloaded file", status_code);
    }

    return http_download_make_success_event(status_code);
}
