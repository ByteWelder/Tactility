#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialization method for TactilityC
 * @warning This is called from the main firmware. Don't call this from an external app!
 */
void tt_init_tactility_c();

#ifdef __cplusplus
}
#endif
