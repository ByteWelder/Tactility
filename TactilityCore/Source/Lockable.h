#pragma once

#include "Check.h"
#include <memory>

namespace tt {

class ScopedLockableUsage;

class Lockable {
public:
    virtual ~Lockable() = default;

    virtual bool lock(uint32_t timeoutTicks) const = 0;
    virtual bool unlock() const = 0;

    std::unique_ptr<ScopedLockableUsage> scoped() const;
};

class ScopedLockableUsage {

    const Lockable& lockable;
    bool locked = false;

public:

    explicit ScopedLockableUsage(const Lockable& lockable) : lockable(lockable) {}

    ~ScopedLockableUsage() {
        if (locked) {
            tt_check(lockable.unlock());
        }
    }

    bool lock(uint32_t timeout) const {
        return lockable.lock(timeout);
    }
};

}
