#include <Tactility/Tactility.h>
#include <Tactility/file/File.h>
#include <Tactility/network/Http.h>

#ifdef ESP_PLATFORM
#include <Tactility/network/EspHttpClient.h>
#include <esp_sntp.h>
#include <esp_http_client.h>
#endif

namespace tt::network::http {

constexpr auto* TAG = "HTTP";

void download(
    const std::string& url,
    const std::string& certFilePath,
    const std::string &downloadFilePath,
    std::function<void()> onSuccess,
    std::function<void(const char* errorMessage)> onError
) {
    TT_LOG_I(TAG, "Downloading %s to %s", url.c_str(), downloadFilePath.c_str());
#ifdef ESP_PLATFORM
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
            onError("Failed to initialize client");
            return -1;
        }

        if (!client->open()) {
            onError("Failed to open connection");
            return -1;
        }

        if (!client->fetchHeaders()) {
            onError("Failed to get request headers");
            return -1;
        }

        if (!client->isStatusCodeOk()) {
            onError("Server response is not OK");
            return -1;
        }

        auto bytes_left = client->getContentLength();

        auto lock = file::getLock(downloadFilePath)->asScopedLock();
        lock.lock();
        auto file_exists = file::isFile(downloadFilePath);
        auto* file_mode = file_exists ? "r+" : "w";
        TT_LOG_I(TAG, "opening %s with mode %s", downloadFilePath.c_str(), file_mode);
        auto* file = fopen(downloadFilePath.c_str(), file_mode);
        if (file == nullptr) {
            onError("Failed to open file");
            return -1;
        }

        TT_LOG_I(TAG, "Writing %d bytes to %s", bytes_left, downloadFilePath.c_str());
        char buffer[512];
        while (bytes_left > 0) {
            int data_read = client->read(buffer, 512);
            if (data_read <= 0) {
                fclose(file);
                onError("Failed to read data");
                return -1;
            }
            bytes_left -= data_read;
            if (fwrite(buffer, 1, data_read, file) != data_read) {
                fclose(file);
                onError("Failed to write all bytes");
                return -1;
            }
        }
        fclose(file);
        TT_LOG_I(TAG, "Downloaded %s to %s", url.c_str(), downloadFilePath.c_str());
        onSuccess();
        return 0;
    });
#else
    getMainDispatcher().dispatch([onError] {
        onError("Not implemented");
    });
#endif
}

}
