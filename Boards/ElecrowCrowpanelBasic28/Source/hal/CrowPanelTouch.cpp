#include "CrowPanelTouch.h"
#include "CrowPanelDisplayConstants.h"

std::shared_ptr<Xpt2046Touch> createTouch() {
    auto configuration = std::make_unique<Xpt2046Touch::Configuration>(
        CROWPANEL_LCD_SPI_HOST,
        GPIO_NUM_33,
        240,
        320,
        false,
        true,
        false
    );

    return std::make_shared<Xpt2046Touch>(std::move(configuration));
}
