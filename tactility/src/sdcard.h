#pragma once

#include "tactility_core.h"

#define TT_SDCARD_MOUNT_POINT "/sdcard"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* (*SdcardMount)(const char* mount_path);
typedef void (*SdcardUnmount)(void*);

typedef enum {
    SDCARD_MOUNT_BEHAVIOUR_AT_BOOT, /** Only mount at boot */
    SDCARD_MOUNT_BEHAVIOUR_ANYTIME /** Mount/dismount any time */
} SdcardMountBehaviour;

typedef struct {
    SdcardMount mount;
    SdcardUnmount unmount;
    SdcardMountBehaviour mount_behaviour;
} Sdcard;

bool tt_sdcard_mount(const Sdcard* sdcard);
bool tt_sdcard_is_mounted();
bool tt_sdcard_unmount();

#ifdef __cplusplus
}
#endif