#pragma once

#include <Tactility/app/App.h>
#include <Tactility/Bundle.h>

namespace tt::app::timezone {

LaunchId start();

std::string getResultName(const Bundle& bundle);
std::string getResultCode(const Bundle& bundle);

}
