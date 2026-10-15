// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <TactilityCpp/Allocator.h>

#include <cstdint>
#include <string>
#include <vector>


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

/** @return the platform name of this device in the catalog's target platforms, e.g. "esp32s3" or "posix-x86_64" */
const char* app_hub_catalog_get_platform();

/** @return true when this device can install and run the entry's app (target platform, device and RAM requirements) */
bool app_hub_catalog_is_compatible(const AppHubCatalogEntry& entry);

/** @return the path of the certificate that verifies the catalog server, in the "system" file system, or an empty string when it isn't available */
std::string app_hub_catalog_get_certificate_path();

std::string app_hub_catalog_get_url();

std::string app_hub_catalog_get_download_url(const std::string& file);
