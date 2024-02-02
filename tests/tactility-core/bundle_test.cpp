#include "doctest.h"
#include "bundle.h"
#include <cstring>

TEST_CASE("boolean can be stored and retrieved") {
    Bundle bundle = tt_bundle_alloc();
    tt_bundle_put_bool(bundle, "key", true);
    CHECK(tt_bundle_get_bool(bundle, "key"));
    bool opt_result = false;
    CHECK(tt_bundle_opt_bool(bundle, "key", &opt_result));
    CHECK(tt_bundle_has_bool(bundle, "key"));
    CHECK_EQ(opt_result, true);
    tt_bundle_free(bundle);
}

TEST_CASE("int32 can be stored and retrieved") {
    Bundle bundle = tt_bundle_alloc();
    tt_bundle_put_int32(bundle, "key", 42);
    CHECK(tt_bundle_get_int32(bundle, "key"));
    int32_t opt_result = 0;
    CHECK(tt_bundle_opt_int32(bundle, "key", &opt_result));
    CHECK(tt_bundle_has_int32(bundle, "key"));
    CHECK_EQ(opt_result, 42);
    tt_bundle_free(bundle);
}

TEST_CASE("string can be stored and retrieved") {
    Bundle bundle = tt_bundle_alloc();
    tt_bundle_put_string(bundle, "key", "value");
    const char* value_from_bundle = tt_bundle_get_string(bundle, "key");
    CHECK_EQ(strcmp(value_from_bundle, "value"), 0);
    char* opt_result = NULL;
    CHECK(tt_bundle_opt_string(bundle, "key", &opt_result));
    CHECK(tt_bundle_has_string(bundle, "key"));
    CHECK(opt_result != NULL);
    tt_bundle_free(bundle);
}

TEST_CASE("bundle copy holds all copied values when original is freed") {
    Bundle original = tt_bundle_alloc();
    tt_bundle_put_bool(original, "bool", true);
    tt_bundle_put_int32(original, "int32", 123);
    tt_bundle_put_string(original, "string", "text");

    Bundle copy = tt_bundle_alloc_copy(original);
    tt_bundle_free(original);

    CHECK(tt_bundle_get_bool(copy, "bool") == true);
    CHECK_EQ(tt_bundle_get_int32(copy, "int32"),  123);
    CHECK_EQ(strcmp(tt_bundle_get_string(copy, "string"), "text"), 0);
    tt_bundle_free(copy);
}
