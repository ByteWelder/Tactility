#pragma once
#ifdef ESP_PLATFORM

namespace tt::service::development {

void setEnableOnBoot(bool enable);

bool shouldEnableOnBoot();

}

#endif // ESP_PLATFORM
