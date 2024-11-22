#pragma once

#include "TactilityCore.h"

namespace tt::hal::sdcard {

#define TT_SDCARD_MOUNT_POINT "/sdcard"

typedef void* (*Mount)(const char* mount_path);
typedef void (*Unmount)(void* context);
typedef bool (*IsMounted)(void* context);

typedef enum {
    StateMounted,
    StateUnmounted,
    StateError,
} State;

typedef enum {
    MountBehaviourAtBoot, /** Only mount at boot */
    MountBehaviourAnytime /** Mount/dismount any time */
} MountBehaviour;

typedef struct {
    Mount mount;
    Unmount unmount;
    IsMounted is_mounted;
    MountBehaviour mount_behaviour;
} SdCard;

bool mount(const SdCard* sdcard);
State get_state();
bool unmount(uint32_t timeout_ticks);

} // namespace
