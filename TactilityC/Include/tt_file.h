#pragma once

#include "tt_lock.h"

#ifdef __cplusplus
extern "C" {
#endif

LockHandle tt_lock_alloc_for_file(const char* path);

#ifdef __cplusplus
}
#endif
