#include "Tactility/Lockable.h"

namespace tt {

std::unique_ptr<ScopedLockableUsage> Lockable::scoped() const {
    auto* scoped = new ScopedLockableUsage(*this);
    return std::unique_ptr<ScopedLockableUsage>(scoped);
}

}
