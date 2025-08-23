#pragma once

#include <Tactility/Lock.h>

#include <memory>
#include <string>

namespace tt::file {

/**
 * @return a non-null lock for the specified path.
 */
std::shared_ptr<Lock> getLock(const std::string& path);

/**
 * Acquires an SD card lock if the path is an SD card path.
 * Always calls the function, but doesn't lock if the path is not an SD card path.
 */
template<typename ReturnType>
ReturnType withLock(const std::string& path, std::function<ReturnType()> fn) {
    const auto lock = getLock(path)->asScopedLock();
    lock.lock();
    return fn();
}

}
