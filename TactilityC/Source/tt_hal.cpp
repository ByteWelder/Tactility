#include "tt_hal.h"

#include <Tactility/Tactility.h>
#include <Tactility/hal/Configuration.h>

extern "C" {

UiScale tt_hal_configuration_get_ui_scale() {
    auto scale = tt::hal::getConfiguration()->uiScale;
    return static_cast<UiScale>(scale);
}

}
