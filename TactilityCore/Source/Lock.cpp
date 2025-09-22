#include "Tactility/Lock.h"

namespace tt {

ScopedLock Lock::asScopedLock() const {
    return ScopedLock(*this);
}

}
