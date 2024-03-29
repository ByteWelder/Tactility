#pragma once

#include "tactility_core.h"

#define TT_SDCARD_MOUNT_POINT "/sdcard"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* (*SdcardMount)(const char* mount_path);
typedef void (*SdcardUnmount)(void* context);
typedef bool (*SdcardIsMounted)(void* context);

typedef enum {
    SdcardStateMounted,
    SdcardStateUnmounted,
    SdcardStateError,
} SdcardState;

typedef enum {
    SdcardMountBehaviourAtBoot, /** Only mount at boot */
    SdcardMountBehaviourAnytime /** Mount/dismount any time */
} SdcardMountBehaviour;

typedef struct {
    SdcardMount mount;
    SdcardUnmount unmount;
    SdcardIsMounted is_mounted;
    SdcardMountBehaviour mount_behaviour;
} SdCard;

bool tt_sdcard_mount(const SdCard* sdcard);
SdcardState tt_sdcard_get_state();
bool tt_sdcard_unmount();

#ifdef __cplusplus
}
#endif