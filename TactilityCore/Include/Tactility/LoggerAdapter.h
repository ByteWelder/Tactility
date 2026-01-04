#pragma once

#include "LogCommon.h"
#include <functional>

namespace tt {

typedef std::function<void(LogLevel, const char* tag, const char*)> LoggerAdapter;

}
