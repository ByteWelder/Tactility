#include <Tactility/StringUtils.h>
#include <Tactility/network/HttpdReq.h>

#include <ranges>

namespace tt::network {

std::map<std::string, std::string> parseContentDisposition(const std::vector<std::string>& input) {
    std::map<std::string, std::string> result;
    static std::string prefix = "Content-Disposition: ";

    // Find header
    auto content_disposition_header = std::ranges::find_if(input, [](const std::string& header) {
        return header.starts_with(prefix);
    });

    // Header not found
    if (content_disposition_header == input.end()) {
        return result;
    }

    auto parseable = content_disposition_header->substr(prefix.size());
    auto parts = string::split(parseable, "; ");
    for (const auto& part : parts) {
        auto key_value = string::split(part, "=");
        if (key_value.size() == 2) {
            // Trim trailing newlines
            auto value = string::trim(key_value[1], "\r\n");
            if (value.size() > 2) {
                result[key_value[0]] = value.substr(1, value.size() - 2);
            } else {
                result[key_value[0]] = "";
            }
        }
    }

    return result;
}

}
