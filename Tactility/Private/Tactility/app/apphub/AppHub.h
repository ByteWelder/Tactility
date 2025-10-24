#pragma once

#include <string>

namespace tt::app::apphub {

constexpr auto* CERTIFICATE_PATH = "/system/certificates/WE1.pem";

std::string getAppsJsonUrl();

std::string getDownloadUrl(const std::string& relativePath);

}