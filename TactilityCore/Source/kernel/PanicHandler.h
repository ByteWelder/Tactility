#pragma once

#include <cstdio>

#define CRASH_DATA_CALLSTACK_LIMIT 32 // bytes

struct CallstackFrame {
    uint32_t pc = 0;
    uint32_t sp = 0;
};

struct CrashData {
    bool callstackCorrupted = false;
    uint8_t callstackLength = 0;
    CallstackFrame callstack[CRASH_DATA_CALLSTACK_LIMIT];
};

const CrashData* getRtcCrashData();
