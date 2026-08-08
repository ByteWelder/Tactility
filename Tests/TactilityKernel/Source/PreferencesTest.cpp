#include "doctest.h"
#include <tactility/preferences.h>

#include <cstdio>
#include <cstring>

namespace {

const char* TEST_PATH = "/tmp/tactility_kernel_preferences_test.properties";

struct ScratchFile {
    ScratchFile() { std::remove(TEST_PATH); }
    ~ScratchFile() { std::remove(TEST_PATH); }
};

bool file_exists(const char* path) {
    FILE* file = std::fopen(path, "r");
    if (file == nullptr) {
        return false;
    }
    std::fclose(file);
    return true;
}

} // namespace

TEST_CASE("preferences_open_path on a missing file starts out empty, without creating it") {
    ScratchFile scratch;

    Preferences* preferences = preferences_open(TEST_PATH);
    CHECK_NE(preferences, nullptr);
    CHECK_FALSE(preferences_has_bool(preferences, "key"));
    CHECK_FALSE(file_exists(TEST_PATH));

    preferences_close(preferences);
}

TEST_CASE("put_*/has_*/opt_* round-trip all four types") {
    ScratchFile scratch;

    Preferences* preferences = preferences_open(TEST_PATH);
    preferences_put_bool(preferences, "flag", true);
    preferences_put_int32(preferences, "count", -42);
    preferences_put_int64(preferences, "big", 123456789012345LL);
    preferences_put_string(preferences, "text", "hello world");

    CHECK(preferences_has_bool(preferences, "flag"));
    bool bool_out = false;
    CHECK(preferences_opt_bool(preferences, "flag", &bool_out));
    CHECK_EQ(bool_out, true);

    CHECK(preferences_has_int32(preferences, "count"));
    int32_t int32_out = 0;
    CHECK(preferences_opt_int32(preferences, "count", &int32_out));
    CHECK_EQ(int32_out, -42);

    CHECK(preferences_has_int64(preferences, "big"));
    int64_t int64_out = 0;
    CHECK(preferences_opt_int64(preferences, "big", &int64_out));
    CHECK_EQ(int64_out, 123456789012345LL);

    CHECK(preferences_has_string(preferences, "text"));
    char buffer[32];
    CHECK_EQ(preferences_opt_string(preferences, "text", buffer, sizeof(buffer)), ERROR_NONE);
    CHECK_EQ(std::strcmp(buffer, "hello world"), 0);

    preferences_close(preferences);
}

TEST_CASE("opt_string reports ERROR_BUFFER_OVERFLOW and ERROR_NOT_FOUND") {
    ScratchFile scratch;

    Preferences* preferences = preferences_open(TEST_PATH);
    preferences_put_string(preferences, "text", "hello world");

    char tiny[4];
    CHECK_EQ(preferences_opt_string(preferences, "text", tiny, sizeof(tiny)), ERROR_BUFFER_OVERFLOW);

    char buffer[32];
    CHECK_EQ(preferences_opt_string(preferences, "missing", buffer, sizeof(buffer)), ERROR_NOT_FOUND);

    preferences_close(preferences);
}

TEST_CASE("has_*/opt_* reject a key stored with a different type") {
    ScratchFile scratch;

    Preferences* preferences = preferences_open(TEST_PATH);
    preferences_put_bool(preferences, "key", true);

    CHECK_FALSE(preferences_has_int32(preferences, "key"));
    CHECK_FALSE(preferences_has_int64(preferences, "key"));
    CHECK_FALSE(preferences_has_string(preferences, "key"));

    int32_t out = 0;
    CHECK_FALSE(preferences_opt_int32(preferences, "key", &out));

    preferences_close(preferences);
}

TEST_CASE("a string value with embedded newlines and backslashes survives a reopen") {
    ScratchFile scratch;

    {
        Preferences* preferences = preferences_open(TEST_PATH);
        preferences_put_string(preferences, "text", "line1\nline2 with \\ backslash");
        preferences_close(preferences);
    }
    {
        Preferences* preferences = preferences_open(TEST_PATH);
        char buffer[64];
        CHECK_EQ(preferences_opt_string(preferences, "text", buffer, sizeof(buffer)), ERROR_NONE);
        CHECK_EQ(std::strcmp(buffer, "line1\nline2 with \\ backslash"), 0);
        preferences_close(preferences);
    }
}

TEST_CASE("preferences_close persists changes, and only close persists them") {
    ScratchFile scratch;

    Preferences* preferences = preferences_open(TEST_PATH);
    preferences_put_bool(preferences, "flag", true);

    // Not persisted yet - only preferences_close() writes to disk.
    CHECK_FALSE(file_exists(TEST_PATH));

    preferences_close(preferences);
    CHECK(file_exists(TEST_PATH));

    Preferences* reopened = preferences_open(TEST_PATH);
    CHECK(preferences_has_bool(reopened, "flag"));
    preferences_close(reopened);
}

TEST_CASE("put_* on an already-closed value is visible without reopening") {
    ScratchFile scratch;

    Preferences* preferences = preferences_open(TEST_PATH);
    preferences_put_int32(preferences, "count", 1);
    preferences_close(preferences);

    Preferences* reopened = preferences_open(TEST_PATH);
    preferences_put_int32(reopened, "count", 2);
    int32_t out = 0;
    CHECK(preferences_opt_int32(reopened, "count", &out));
    CHECK_EQ(out, 2);
    preferences_close(reopened);

    Preferences* final_instance = preferences_open(TEST_PATH);
    CHECK(preferences_opt_int32(final_instance, "count", &out));
    CHECK_EQ(out, 2);
    preferences_close(final_instance);
}
