#pragma once

#include "Check.h"
#include "RtosCompat.h"
#include <memory>
#include <functional>
#include <algorithm>

namespace tt {

class ScopedLockableUsage;

/** Represents a lock/mutex */
class Lockable {
public:
    virtual ~Lockable() = default;

    virtual bool lock(TickType_t timeoutTicks) const = 0;

    virtual bool lock() const { return lock(portMAX_DELAY); }

    virtual bool unlock() const = 0;

    std::unique_ptr<ScopedLockableUsage> scoped() const;
};

/**
 * Represents a lockable instance that is scoped to a specific lifecycle.
 * Once the ScopedLockableUsage is destroyed, unlock() is called automatically.
 *
 * In other words:
 * You have to lock() this object manually, but unlock() happens automatically on destruction.
 */
class ScopedLockableUsage final : public Lockable {

    const Lockable& lockable;

public:

    explicit ScopedLockableUsage(const Lockable& lockable) : lockable(lockable) {}

    ~ScopedLockableUsage() final {
        lockable.unlock(); // We don't care whether it succeeded or not
    }

    bool lock(TickType_t timeout) const override {
        return lockable.lock(timeout);
    }

    bool lock() const override { return lock(portMAX_DELAY); }

    bool unlock() const override {
        return lockable.unlock();
    }
};

}
