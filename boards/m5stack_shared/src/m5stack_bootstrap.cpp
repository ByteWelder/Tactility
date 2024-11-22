#include "M5Unified.hpp"
#include "Log.h"

#define TAG "m5stack_bootstrap"

bool m5stack_bootstrap() {
    TT_LOG_I(TAG, "Initializing M5Unified");
    M5.begin();
    return true;
}
