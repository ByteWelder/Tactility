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

class ScopedLockableUsage final : public Lockable {

    const Lockable& lockable;

public:

    explicit ScopedLockableUsage(const Lockable& lockable) : lockable(lockable) {}

    ~ScopedLockableUsage() final {
        lockable.unlock(); // We don't care whether it succeeded or not
    }

    bool lock(uint32_t timeout) const override {
        return lockable.lock(timeout);
    }

    bool unlock() const override {
        return lockable.unlock();
    }
};

}
