#include "doctest.h"

#include <app_hub/app_hub_catalog.h>

#include <cstdio>
#include <string>
#include <vector>

namespace {

std::string writeAppsJson(const char* contents) {
    const std::string path = "/tmp/tactility_app_hub_catalog_test.json";
    FILE* file = fopen(path.c_str(), "w");
    REQUIRE_NE(file, nullptr);
    fputs(contents, file);
    fclose(file);
    return path;
}

const std::string CATALOG_PATH = "/tmp/tactility_app_hub_catalog_test.txt";

std::vector<AppHubCatalogEntry> readCatalog() {
    std::vector<AppHubCatalogEntry> entries;
    REQUIRE(app_hub_catalog_for_each(CATALOG_PATH.c_str(), [](const AppHubCatalogEntry& entry, void* context) {
        static_cast<std::vector<AppHubCatalogEntry>*>(context)->push_back(entry);
        return true;
    }, &entries));
    return entries;
}

AppHubCatalogEntry entryWith(std::vector<std::string> requiresDeviceId, int32_t requiresRam) {
    AppHubCatalogEntry entry {};
    entry.requires_device_id = std::move(requiresDeviceId);
    entry.requires_ram = requiresRam;
    return entry;
}

} // namespace

TEST_CASE("app hub: converted entries keep all fields") {
    const auto path = writeAppsJson(R"({"apps": [
        {"appId": "a", "appVersionName": "1.2", "appVersionCode": 12, "appName": "A", "appDescription": "Tab\there", "targetSdk": "0.8.0", "targetPlatforms": ["esp32", "esp32s3"], "file": "a-1.app",
         "requiresDeviceId": ["m5stack-tab5", "lilygo-tdeck"], "requiresRam": 2},
        {"appId": "b", "appVersionName": "1", "appVersionCode": 1, "appName": "B", "appDescription": "", "targetSdk": "0.8.0", "targetPlatforms": [], "file": "b-1.app"}
    ]})");
    REQUIRE(app_hub_catalog_convert(path.c_str(), CATALOG_PATH.c_str()));
    const auto entries = readCatalog();
    REQUIRE_EQ(entries.size(), 2u);
    CHECK_EQ(entries[0].id, "a");
    CHECK_EQ(entries[0].version_name, "1.2");
    CHECK_EQ(entries[0].version_code, 12);
    CHECK_EQ(entries[0].name, "A");
    CHECK_EQ(entries[0].description, "Tab here");
    CHECK_EQ(entries[0].target_sdk, "0.8.0");
    CHECK_EQ(entries[0].target_platforms, std::vector<std::string> { "esp32", "esp32s3" });
    CHECK_EQ(entries[0].file, "a-1.app");
    CHECK_EQ(entries[0].requires_device_id, std::vector<std::string> { "m5stack-tab5", "lilygo-tdeck" });
    CHECK_EQ(entries[0].requires_ram, 2);
    CHECK(entries[1].target_platforms.empty());
    CHECK(entries[1].requires_device_id.empty());
    CHECK_EQ(entries[1].requires_ram, 0);
    remove(path.c_str());
    remove(CATALOG_PATH.c_str());
}

TEST_CASE("app hub: a catalog entry is found by id") {
    const auto path = writeAppsJson(R"({"apps": [
        {"appId": "a", "appVersionName": "1", "appVersionCode": 1, "appName": "A", "appDescription": "", "targetSdk": "0.8.0", "targetPlatforms": [], "file": "a-1.app"},
        {"appId": "b", "appVersionName": "1", "appVersionCode": 1, "appName": "B", "appDescription": "", "targetSdk": "0.8.0", "targetPlatforms": [], "file": "b-1.app"}
    ]})");
    REQUIRE(app_hub_catalog_convert(path.c_str(), CATALOG_PATH.c_str()));
    AppHubCatalogEntry entry;
    REQUIRE(app_hub_catalog_find(CATALOG_PATH.c_str(), "b", entry));
    CHECK_EQ(entry.name, "B");
    CHECK_FALSE(app_hub_catalog_find(CATALOG_PATH.c_str(), "c", entry));
    remove(path.c_str());
    remove(CATALOG_PATH.c_str());
}

TEST_CASE("app hub: a visitor can stop the visit") {
    const auto path = writeAppsJson(R"({"apps": [
        {"appId": "a", "appVersionName": "1", "appVersionCode": 1, "appName": "A", "appDescription": "", "targetSdk": "0.8.0", "targetPlatforms": [], "file": "a-1.app"},
        {"appId": "b", "appVersionName": "1", "appVersionCode": 1, "appName": "B", "appDescription": "", "targetSdk": "0.8.0", "targetPlatforms": [], "file": "b-1.app"}
    ]})");
    REQUIRE(app_hub_catalog_convert(path.c_str(), CATALOG_PATH.c_str()));
    int visited = 0;
    CHECK(app_hub_catalog_for_each(CATALOG_PATH.c_str(), [](const AppHubCatalogEntry&, void* context) {
        ++*static_cast<int*>(context);
        return false;
    }, &visited));
    CHECK_EQ(visited, 1);
    remove(path.c_str());
    remove(CATALOG_PATH.c_str());
}

TEST_CASE("app hub: a failed conversion leaves no catalog file") {
    const auto fractional = writeAppsJson(R"({"apps": [
        {"appId": "a", "appVersionName": "1", "appVersionCode": 1, "appName": "A", "appDescription": "", "targetSdk": "0.8.0", "targetPlatforms": [], "file": "a-1.app",
         "requiresRam": 1.5}
    ]})");
    CHECK_FALSE(app_hub_catalog_convert(fractional.c_str(), CATALOG_PATH.c_str()));
    CHECK_EQ(fopen(CATALOG_PATH.c_str(), "rb"), nullptr);
    const auto tooLong = writeAppsJson((R"({"apps": [{"appId": "a", "appVersionName": "1", "appVersionCode": 1, "appName": "A", "appDescription": ")" + std::string(2000, 'x') + R"(", "targetSdk": "0.8.0", "targetPlatforms": [], "file": "a-1.app"}]})").c_str());
    CHECK_FALSE(app_hub_catalog_convert(tooLong.c_str(), CATALOG_PATH.c_str()));
    CHECK_EQ(fopen(CATALOG_PATH.c_str(), "rb"), nullptr);
    CHECK_FALSE(app_hub_catalog_for_each(CATALOG_PATH.c_str(), [](const AppHubCatalogEntry&, void*) { return true; }, nullptr));
    remove(fractional.c_str());
}

TEST_CASE("app hub: isCompatible applies the device and RAM requirements") {
    CHECK(app_hub_catalog_is_compatible(entryWith({}, 0)));
    CHECK(app_hub_catalog_is_compatible(entryWith({ "other-device", CONFIG_TT_DEVICE_ID }, 1)));
    CHECK_FALSE(app_hub_catalog_is_compatible(entryWith({ "other-device" }, 0)));
    CHECK_FALSE(app_hub_catalog_is_compatible(entryWith({}, 256)));
}

TEST_CASE("app hub: isCompatible requires this device's platform when the app lists platforms") {
    auto entry = entryWith({}, 0);
    CHECK(app_hub_catalog_is_compatible(entry));
    entry.target_platforms = { "some-other-platform" };
    CHECK_FALSE(app_hub_catalog_is_compatible(entry));
    entry.target_platforms = { "some-other-platform", app_hub_catalog_get_platform() };
    CHECK(app_hub_catalog_is_compatible(entry));
}
