// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <TactilityCpp/Allocator.h>

#include <cstdint>
#include <string>
#include <vector>

constexpr auto* APP_HUB_CERTIFICATE_PATH = "/system/certificates/WE1.pem";

struct AppHubCatalogEntry {
    std::string id;
    std::string version_name;
    int32_t version_code = 0;
    std::string name;
    std::string description;
    std::string target_sdk;
    std::vector<std::string> target_platforms;
    /** Device ids the app is restricted to. Empty means unrestricted. */
    std::vector<std::string> requires_device_id;
    /** RAM in megabytes the app needs. 0 means no requirement. */
    int32_t requires_ram = 0;
    /** Path of the package, relative to the catalog's base URL */
    std::string file;
};

using AppHubCatalog = std::vector<AppHubCatalogEntry, tt::OptExternalAllocator<AppHubCatalogEntry>>;

/** Parse an apps.json file */
bool app_hub_catalog_parse(const char* path, AppHubCatalog& catalog);

/** @return true when this device can install and run the entry's app (target platform, device and RAM requirements) */
bool app_hub_catalog_is_compatible(const AppHubCatalogEntry& entry);

std::string app_hub_catalog_get_url();

std::string app_hub_catalog_get_download_url(const std::string& file);
