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

AppHubCatalogEntry entryWith(std::vector<std::string> requiresDeviceId, int32_t requiresRam) {
    AppHubCatalogEntry entry {};
    entry.requires_device_id = std::move(requiresDeviceId);
    entry.requires_ram = requiresRam;
    return entry;
}

} // namespace

TEST_CASE("app hub: requiresDeviceId and requiresRam are parsed, and optional") {
    const auto path = writeAppsJson(R"({"apps": [
        {"appId": "a", "appVersionName": "1", "appVersionCode": 1, "appName": "A", "appDescription": "", "targetSdk": "0.8.0", "targetPlatforms": [], "file": "a-1.app",
         "requiresDeviceId": ["m5stack-tab5", "lilygo-tdeck"], "requiresRam": 2},
        {"appId": "b", "appVersionName": "1", "appVersionCode": 1, "appName": "B", "appDescription": "", "targetSdk": "0.8.0", "targetPlatforms": [], "file": "b-1.app"}
    ]})");
    AppHubCatalog entries;
    REQUIRE(app_hub_catalog_parse(path.c_str(), entries));
    REQUIRE_EQ(entries.size(), 2u);
    CHECK_EQ(entries[0].requires_device_id, std::vector<std::string> { "m5stack-tab5", "lilygo-tdeck" });
    CHECK_EQ(entries[0].requires_ram, 2);
    CHECK(entries[1].requires_device_id.empty());
    CHECK_EQ(entries[1].requires_ram, 0);
    remove(path.c_str());
}

TEST_CASE("app hub: reparsing into the same list resets optional requirements") {
    AppHubCatalog entries;
    const auto restricted = writeAppsJson(R"({"apps": [
        {"appId": "a", "appVersionName": "1", "appVersionCode": 1, "appName": "A", "appDescription": "", "targetSdk": "0.8.0", "targetPlatforms": [], "file": "a-1.app",
         "requiresDeviceId": ["m5stack-tab5"], "requiresRam": 2}
    ]})");
    REQUIRE(app_hub_catalog_parse(restricted.c_str(), entries));
    const auto unrestricted = writeAppsJson(R"({"apps": [
        {"appId": "b", "appVersionName": "1", "appVersionCode": 1, "appName": "B", "appDescription": "", "targetSdk": "0.8.0", "targetPlatforms": [], "file": "b-1.app"}
    ]})");
    REQUIRE(app_hub_catalog_parse(unrestricted.c_str(), entries));
    REQUIRE_EQ(entries.size(), 1u);
    CHECK(entries[0].requires_device_id.empty());
    CHECK_EQ(entries[0].requires_ram, 0);
    remove(unrestricted.c_str());
}

TEST_CASE("app hub: a fractional requiresRam rejects the entry") {
    const auto path = writeAppsJson(R"({"apps": [
        {"appId": "a", "appVersionName": "1", "appVersionCode": 1, "appName": "A", "appDescription": "", "targetSdk": "0.8.0", "targetPlatforms": [], "file": "a-1.app",
         "requiresRam": 1.5}
    ]})");
    AppHubCatalog entries;
    CHECK_FALSE(app_hub_catalog_parse(path.c_str(), entries));
    remove(path.c_str());
}

TEST_CASE("app hub: isCompatible applies the device and RAM requirements") {
    CHECK(app_hub_catalog_is_compatible(entryWith({}, 0)));
    CHECK(app_hub_catalog_is_compatible(entryWith({ "other-device", CONFIG_TT_DEVICE_ID }, 1)));
    CHECK_FALSE(app_hub_catalog_is_compatible(entryWith({ "other-device" }, 0)));
    CHECK_FALSE(app_hub_catalog_is_compatible(entryWith({}, 256)));
}
