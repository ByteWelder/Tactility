#pragma once

#include <memory>
#include <Tactility/Lock.h>

struct LockHolder {
    std::shared_ptr<tt::Lock> lock;
};
