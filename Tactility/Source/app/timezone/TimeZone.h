#pragma once

#include "Bundle.h"

namespace tt::app::timezone {

void start();

std::string getResultName(const Bundle& bundle);
std::string getResultCode(const Bundle& bundle);

}
