#pragma once

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

#ifdef __cplusplus
}
#endif