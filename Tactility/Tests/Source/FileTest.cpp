#include "doctest.h"
#include <Tactility/file/File.h>

using namespace tt;

TEST_CASE("findOrCreateDirectory can create a directory tree without prefix") {
    CHECK_EQ(file::findOrCreateDirectory("test1/test1", 0777), true);
    CHECK_EQ(file::deleteRecursively("test1"), true);
}

TEST_CASE("findOrCreateDirectory can create a directory tree with prefix") {
    CHECK_EQ(file::findOrCreateDirectory("/tmp/test2", 0777), true);
    CHECK_EQ(file::deleteRecursively("/tmp/test2"), true);
}
