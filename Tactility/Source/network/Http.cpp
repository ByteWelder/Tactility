#include <esp_sntp.h>
#include <Tactility/Tactility.h>
#include <Tactility/file/File.h>
#include <Tactility/network/Http.h>
#include <Tactility/network/EspHttpClient.h>

namespace tt::network::http {

constexpr auto* TAG = "HTTP";

void download(
    const std::string& url,
    const std::string& certFilePath,
    const std::string &downloadFilePath,
    std::function<void()> onSuccess,
    std::function<void()> onError
) {
    TT_LOG_I(TAG, "Download %s to %s", url.c_str(), downloadFilePath.c_str());
    getMainDispatcher().dispatch([url, certFilePath, downloadFilePath, onSuccess, onError] {
        TT_LOG_I(TAG, "Loading certificate");
        auto certificate = file::readString(certFilePath);
        auto certificate_length = strlen(reinterpret_cast<const char*>(certificate.get())) + 1;

        auto config = std::make_unique<esp_http_client_config_t>(esp_http_client_config_t {
            .url = url.c_str(),
            .auth_type = HTTP_AUTH_TYPE_NONE,
            .cert_pem = reinterpret_cast<const char*>(certificate.get()),
            .cert_len = certificate_length,
            .tls_version = ESP_HTTP_CLIENT_TLS_VER_TLS_1_3,
            .method = HTTP_METHOD_GET,
            .timeout_ms = 5000,
            .transport_type = HTTP_TRANSPORT_OVER_SSL
        });

        auto client = std::make_unique<EspHttpClient>();
        if (!client->init(std::move(config))) {
            onError();
            return -1;
        }

        if (!client->open()) {
            onError();
            return -1;
        }

        if (!client->fetchHeaders()) {
            onError();
            return -1;
        }

        if (!client->isStatusCodeOk()) {
            onError();
            return -1;
        }

        auto bytes_left = client->getContentLength();

        auto lock = file::getLock(downloadFilePath)->asScopedLock();
        lock.lock();
        auto file_exists = file::isFile(downloadFilePath);
        auto* file_mode = file_exists ? "r+" : "w";
        auto* file = fopen(downloadFilePath.c_str(), file_mode);

        TT_LOG_I(TAG, "Writing %d bytes to %s", bytes_left, downloadFilePath.c_str());
        char buffer[512];
        while (bytes_left > 0) {
            int data_read = client->read(buffer, 512);
            if (data_read <= 0) {
                onError();
                return -1;
            }
            bytes_left -= data_read;
            if (fwrite(buffer, 1, data_read, file) != data_read) {
                TT_LOG_E(TAG, "Failed to write all bytes");
                fclose(file);
                onError();
                return -1;
            }
        }
        fclose(file);
        onSuccess();
        return 0;
    });
}

}
