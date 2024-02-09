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
    SDCARD_STATE_MOUNTED,
    SDCARD_STATE_UNMOUNTED,
    SDCARD_STATE_ERROR,
} SdcardState;

typedef enum {
    SDCARD_MOUNT_BEHAVIOUR_AT_BOOT, /** Only mount at boot */
    SDCARD_MOUNT_BEHAVIOUR_ANYTIME /** Mount/dismount any time */
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