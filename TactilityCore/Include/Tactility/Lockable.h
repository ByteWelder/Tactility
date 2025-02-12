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

    virtual bool lock(TickType_t timeout) const = 0;

    bool lock() const { return lock(portMAX_DELAY); }

    virtual bool unlock() const = 0;

    void withLock(TickType_t timeout, const std::function<void()>& onLockAcquired) const {
        if (lock(timeout)) {
            onLockAcquired();
            unlock();
        }
    }

    void withLock(TickType_t timeout, const std::function<void()>& onLockAcquired, const std::function<void()>& onLockFailure) const {
        if (lock(timeout)) {
            onLockAcquired();
            unlock();
        } else {
            onLockFailure();
        }
    }

    void withLock(const std::function<void()>& onLockAcquired) const { withLock(portMAX_DELAY, onLockAcquired); }

    void withLock(const std::function<void()>& onLockAcquired, const std::function<void()>& onLockFailed) const { withLock(portMAX_DELAY, onLockAcquired, onLockFailed); }

    [[deprecated("use asScopedLock()")]]
    std::unique_ptr<ScopedLockableUsage> scoped() const;

    ScopedLockableUsage asScopedLock() const;
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

    using Lockable::lock;

    explicit ScopedLockableUsage(const Lockable& lockable) : lockable(lockable) {}

    ~ScopedLockableUsage() final {
        lockable.unlock(); // We don't care whether it succeeded or not
    }

    bool lock(TickType_t timeout) const override {
        return lockable.lock(timeout);
    }

    bool unlock() const override {
        return lockable.unlock();
    }
};

}
