#include "Tactility/Lockable.h"

namespace tt {

std::unique_ptr<ScopedLockableUsage> Lockable::scoped() const {
    return std::make_unique<ScopedLockableUsage>(*this);
}

ScopedLockableUsage Lockable::asScopedLock() const {
    return ScopedLockableUsage(*this);
}

}
