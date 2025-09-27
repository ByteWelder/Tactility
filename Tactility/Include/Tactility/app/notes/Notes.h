#pragma once

#include <Tactility/app/App.h>

namespace tt::app::notes {

/**
 * Start the notes app with the specified text file.
 * @param[in] filePath the path to the text file to open
 * @return the launch id
 */
LaunchId start(const std::string& filePath);

}
