#include "doctest.h"

#include <tactility/drivers/radio.h>
#include <tactility/filesystem/file_system.h>
#include <tactility/radio_settings.h>

#include <cstdio>
#include <cstring>
#include <filesystem>

namespace {

const char* TEST_ROOT = "/tmp/tactility_kernel_radio_settings_test";
const char* SETTINGS_PATH = "/tmp/tactility_kernel_radio_settings_test/tactility/settings/radio.properties";

error_t fake_mount(void*) { return ERROR_NONE; }
error_t fake_unmount(void*) { return ERROR_NONE; }
bool fake_is_mounted(void*) { return false; }

error_t fake_get_path(void* data, char* out_path, size_t out_path_size) {
    const char* path = static_cast<const char*>(data);
    if (std::strlen(path) + 1 > out_path_size) {
        return ERROR_BUFFER_OVERFLOW;
    }
    std::strcpy(out_path, path);
    return ERROR_NONE;
}

const FileSystemApi fake_data_api = {
    .mount = fake_mount,
    .unmount = fake_unmount,
    .is_mounted = fake_is_mounted,
    .get_path = fake_get_path,
};

// Registers a "data" filesystem in a scratch directory for the lifetime of one test case
struct FakeDataFs {
    FileSystem* fs;

    FakeDataFs() {
        std::filesystem::remove_all(TEST_ROOT);
        fs = file_system_add("data", &fake_data_api, const_cast<char*>(TEST_ROOT));
    }

    ~FakeDataFs() {
        file_system_remove(fs);
        std::filesystem::remove_all(TEST_ROOT);
    }
};

void write_settings(const char* content) {
    std::filesystem::create_directories(std::filesystem::path(SETTINGS_PATH).parent_path());
    FILE* file = std::fopen(SETTINGS_PATH, "w");
    REQUIRE(file != nullptr);
    std::fputs(content, file);
    std::fclose(file);
}

} // namespace

TEST_CASE("radio_region_from_string resolves every region name") {
    for (int i = RADIO_REGION_UNSET; i < RADIO_REGION_COUNT; i++) {
        RadioRegion region = RADIO_REGION_COUNT;
        CHECK_EQ(radio_region_from_string(radio_region_to_string(static_cast<RadioRegion>(i)), &region), ERROR_NONE);
        CHECK_EQ(region, static_cast<RadioRegion>(i));
    }
    RadioRegion region = RADIO_REGION_EU_868;
    CHECK_EQ(radio_region_from_string("MARS_123", &region), ERROR_NOT_FOUND);
    CHECK_EQ(region, RADIO_REGION_EU_868);
}

TEST_CASE("radio_settings_load_region is unset without a settings file") {
    FakeDataFs fake;
    CHECK_EQ(radio_settings_load_region(), RADIO_REGION_UNSET);
}

TEST_CASE("radio_settings_save_region persists the region") {
    FakeDataFs fake;
    CHECK_EQ(radio_settings_save_region(RADIO_REGION_US_915), ERROR_NONE);
    CHECK_EQ(radio_settings_load_region(), RADIO_REGION_US_915);
}

TEST_CASE("radio_settings_load_region reads the existing file format") {
    FakeDataFs fake;
    write_settings("region=EU_868\n");
    CHECK_EQ(radio_settings_load_region(), RADIO_REGION_EU_868);
}

TEST_CASE("radio_settings_load_region ignores an unknown region") {
    FakeDataFs fake;
    write_settings("region=MARS_123\n");
    CHECK_EQ(radio_settings_load_region(), RADIO_REGION_UNSET);
}

TEST_CASE("radio_settings_save_region fails without user data storage") {
    CHECK_EQ(radio_settings_save_region(RADIO_REGION_US_915), ERROR_NOT_FOUND);
}
