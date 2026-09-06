#pragma once

#include <map>
#include <string>
#include <vector>

namespace tt::network {

/** Pure string parsing, no request I/O */
std::map<std::string, std::string> parseContentDisposition(const std::vector<std::string>& input);

}
