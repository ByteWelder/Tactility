#pragma once

#include "app/instance.h"

#include <string>

/**
 * Show a QR code over the whole app area.
 */
namespace tt::app::qrcode {

/**
 * Show a QR code of @a text. It closes on a touch, a key press or encoder input.
 * @return the new app's instance id, or 0 when it couldn't be started
 */
AppInstanceId start(const std::string& text);

}
