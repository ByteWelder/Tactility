#pragma once

#include "LoggerCommon.h"
#include <functional>

namespace tt {

typedef std::function<void(LogLevel level, const char* tag, const char* message)> LoggerAdapter;

}
