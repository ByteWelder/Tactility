#pragma once

#include <string>
#include <vector>
#include "Bundle.h"

/**
 * Start the app by its ID and provide:
 *  - an optional title
 *  - 2 or more items
 *
 *  If you provide 0 items, the app will auto-close.
 *  If you provide 1 item, the app will auto-close with result index 0
 */
namespace tt::app::selectiondialog {

    /** App startup parameters */

    void setTitleParameter(Bundle& bundle, const std::string& title);
    void setItemsParameter(Bundle& bundle, const std::vector<std::string>& items);

    /** App result data */

    int32_t getResultIndex(const Bundle& bundle);
}