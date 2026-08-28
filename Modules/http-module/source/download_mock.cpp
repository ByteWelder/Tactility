// SPDX-License-Identifier: Apache-2.0
#include <http/private/download.h>

HttpDownloadEvent http_download_run(const std::string&, const std::string&, const std::string&, HttpDownloadLink* link) {
    if (http_download_is_cancelled(link)) {
        return http_download_make_cancelled_event();
    }
    return http_download_make_error_event("HTTP downloads are not supported on this platform");
}
