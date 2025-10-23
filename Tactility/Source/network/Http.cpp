#include <esp_sntp.h>
#include <Tactility/Tactility.h>
#include <Tactility/file/File.h>
#include <Tactility/network/Http.h>

namespace tt::network::http {

constexpr auto* TAG = "HTTP";

void download(const std::string& url, const std::string& certFilePath, const std::string &downloadFilePath) {
    TT_LOG_I(TAG, "Download %s to %s", url.c_str(), downloadFilePath.c_str());
    getMainDispatcher().dispatch([url, certFilePath, downloadFilePath] {
        auto certificate = file::readString(certFilePath);
        auto certificate_length = strlen(reinterpret_cast<const char*>(certificate.get())) + 1;
        TT_LOG_I(TAG, "Certificate loaded");

        esp_http_client_config_t config = {
            .url = url.c_str(),
            .auth_type = HTTP_AUTH_TYPE_NONE,
            .cert_pem = reinterpret_cast<const char*>(certificate.get()),
            .cert_len = certificate_length,
            .tls_version = ESP_HTTP_CLIENT_TLS_VER_TLS_1_3,
            .method = HTTP_METHOD_GET,
            .timeout_ms = 5000,
            .transport_type = HTTP_TRANSPORT_OVER_SSL
        };

        TT_LOG_I(TAG, "Request init");
        auto client = esp_http_client_init(&config);
        if (client == nullptr) {
            TT_LOG_E(TAG, "Failed to create client");
            return -1;
        }

        TT_LOG_I(TAG, "Request opening");
        if (esp_http_client_open(client, 0) != ESP_OK) {
            TT_LOG_E(TAG, "Failed to open");
            return -1;
        }

        TT_LOG_I(TAG, "Fetching headers");
        if (esp_http_client_fetch_headers(client) < 0) {
            TT_LOG_E(TAG, "Failed to fetch headers");
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return -1;
        }

        auto status_code = esp_http_client_get_status_code(client);
        if (status_code < 200 || status_code >= 300) {
            TT_LOG_E(TAG, "Status code %d", status_code);
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return -1;
        }

        auto bytes_left = esp_http_client_get_content_length(client);
        TT_LOG_I(TAG, "Fetching %d bytes", bytes_left);

        auto lock = file::getLock(downloadFilePath)->asScopedLock();
        lock.lock();
        auto file_exists = file::isFile(downloadFilePath);
        auto* file_mode = file_exists ? "r+" : "w";
        auto* file = fopen(downloadFilePath.c_str(), file_mode);

        char buffer[512];
        while (bytes_left > 0) {
            int data_read = esp_http_client_read(client, buffer, 512);
            if (data_read <= 0) {
                esp_http_client_close(client);
                esp_http_client_cleanup(client);
                return -1;
            }
            bytes_left -= data_read;
            if (fwrite(buffer, 1, data_read, file) != data_read) {
                TT_LOG_E(TAG, "Failed to write all bytes");
                fclose(file);
                esp_http_client_close(client);
                esp_http_client_cleanup(client);
                return -1;
            }
        }
        fclose(file);

        // esp_http_client_read(client);
        TT_LOG_I(TAG, "Request closing");
        esp_http_client_close(client);
        TT_LOG_I(TAG, "Request cleanup");
        esp_http_client_cleanup(client);
        TT_LOG_I(TAG, "Request done");
        return 0;
    });
}

}
