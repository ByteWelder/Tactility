#pragma once

#include <Tactility/Lock.h>

#include <memory>
#include <string>

/**
 * Some file systems belong to devices on a shared bus (e.g. SPI SD card).
 * Because of the shared bus, a lock is required for its operation.
 */
namespace tt::file {

/**
 * @param[in] path the path to find a lock for
 * @return a non-null lock for the specified path.
 */
std::shared_ptr<Lock> getLock(const std::string& path);

/**
 * Acquires a lock, calls the function, then releases the lock.
 * @param[in] path the path to find a lock for
 * @param[in] fn the code to execute while the lock is acquired
 */
template<typename ReturnType>
ReturnType withLock(const std::string& path, std::function<ReturnType()> fn) {
    const auto lock = getLock(path)->asScopedLock();
    lock.lock();
    return fn();
}

}
