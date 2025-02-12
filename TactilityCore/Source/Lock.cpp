#include "Tactility/Lock.h"

namespace tt {

std::unique_ptr<ScopedLock> Lock::scoped() const {
    return std::make_unique<ScopedLock>(*this);
}

ScopedLock Lock::asScopedLock() const {
    return ScopedLock(*this);
}

}
