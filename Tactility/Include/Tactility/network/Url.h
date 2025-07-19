#pragma once

#include <map>
#include <string>

namespace tt::network {

/**
 * Parse a query from a URL
 * @param[in] query
 * @return a map with key-values
 */
std::map<std::string, std::string> parseUrlQuery(std::string query);

std::string urlEncode(const std::string& input);

std::string urlDecode(const std::string& input);

} // namespace