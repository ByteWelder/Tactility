#include "M5Unified.hpp"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG "m5stack_bootstrap"

bool m5stack_bootstrap() {
    TT_LOG_I(TAG, "Initializing M5Unified");
    M5.begin();
    return true;
}

#ifdef __cplusplus
}
#endif
