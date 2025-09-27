#pragma once

#include <Tactility/app/App.h>
#include <Tactility/Bundle.h>

#include <string>
#include <vector>

/**
 * Start the app by its ID and provide:
 *  - an optional title
 *  - 2 or more items
 *
 *  If you provide 0 items, the app will auto-close.
 *  If you provide 1 item, the app will auto-close with result index 0
 */
namespace tt::app::selectiondialog {

LaunchId start(const std::string& title, const std::vector<std::string>& items);

/**
 * Get the index of the item that the user selected.
 *
 * @return a value greater than 0 when a selection was done, or -1 when the app was closed without selecting an item.
 */
int32_t getResultIndex(const Bundle& bundle);

}
