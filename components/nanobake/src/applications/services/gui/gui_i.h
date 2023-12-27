#pragma once

#include "gui.h"
#include <mutex.h>
#include <m-dict.h>
#include <m-core.h>

typedef struct {
    ScreenId id;
    lv_obj_t* parent;
    InitScreen _Nonnull callback;
} NbScreen;

DICT_DEF2(screen_dict, ScreenId, M_BASIC_OPLIST, NbScreen, M_POD_OPLIST)

struct NbGui {
    // TODO: use mutex
    FuriMutex* mutex;
    screen_dict_t screens;
};
