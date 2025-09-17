#pragma once

#include <Tactility/Bundle.h>

#include <string>

/**
 * Start the app by its ID and provide:
 *  - a title
 *  - a text
 */
namespace tt::app::inputdialog {

void start(const std::string& title, const std::string& message, const std::string& prefilled = "");

/**
 * @return the text that was in the field when OK was pressed, or otherwise empty string
 */
std::string getResult(const Bundle& bundle);
}
