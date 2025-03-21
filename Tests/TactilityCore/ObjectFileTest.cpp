#include "doctest.h"
#include <Tactility/file/ObjectFile.h>

using tt::file::ObjectFileWriter;
using tt::file::ObjectFileReader;

constexpr const char* TEMP_FILE = "test.tmp";

struct TestStruct {
    int a;
    bool b;
    char c[4];
};

TEST_CASE("Writing and reading multiple records to a file") {
    ObjectFileWriter writer = ObjectFileWriter(TEMP_FILE, sizeof(TestStruct), 1);

    TestStruct record_out_1 = {
        .a = 0,
        .b = true,
        .c = "123"
    };

    TestStruct record_out_2 = {
        .a = -1,
        .b = false,
        .c = "000"
    };

    CHECK_EQ(writer.open(), true);
    CHECK_EQ(writer.write(&record_out_1), true);
    CHECK_EQ(writer.write(&record_out_2), true);
    writer.close();

    TestStruct record_in;
    ObjectFileReader reader = ObjectFileReader(TEMP_FILE, sizeof(TestStruct));
    CHECK_EQ(reader.open(), true);
    CHECK_EQ(reader.hasNext(), true);
    CHECK_EQ(reader.readNext(&record_in), true);
    CHECK_EQ(reader.hasNext(), true);
    CHECK_EQ(reader.readNext(&record_in), true);
    CHECK_EQ(reader.hasNext(), false);
    reader.close();

    CHECK_EQ(remove(TEMP_FILE), 0);
}

