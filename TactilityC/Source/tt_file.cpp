#include <tt_file.h>
#include <tt_lock_private.h>
#include <Tactility/file/File.h>

extern "C" {

LockHandle tt_lock_alloc_for_file(const char* path) {
    auto lock = tt::file::getLock(path);
    auto holder = new LockHolder(lock);
    return holder;
}

}
