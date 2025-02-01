#pragma once

#include <Tactility/Bundle.h>

#include <string>
#include <vector>

/**
 * Start the app by its ID and provide:
 *  - a title
 *  - a text
 *  - 0, 1 or more buttons
 */
namespace tt::app::alertdialog {

    /**
     * Show a dialog with the provided title, message and 0, 1 or more buttons.
     * @param[in] title the title to show in the toolbar
     * @param[in] message the message to display
     * @param[in] buttonLabels the buttons to show
     */
    void start(const std::string& title, const std::string& message, const std::vector<std::string>& buttonLabels);

    /**
     * Get the index of the button that the user selected.
     *
     * @return a value greater than 0 when a selection was done, or -1 when the app was closed clicking one of the selection buttons.
     */
    int32_t getResultIndex(const Bundle& bundle);
}
