#pragma once

#include <string>
#include <vector>
#include "Bundle.h"

/**
 * Start the app by its ID and provide:
 *  - a title
 *  - a text
 *  - 0, 1 or more buttons
 */
namespace tt::app::alertdialog {

    void start(std::string title, std::string message, const std::vector<std::string>& buttonLabels);

    /**
     * Get the index of the button that the user selected.
     *
     * @return a value greater than 0 when a selection was done, or -1 when the app was closed clicking one of the selection buttons.
     */
    int32_t getResultIndex(const Bundle& bundle);
}
