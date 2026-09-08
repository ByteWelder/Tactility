// SPDX-License-Identifier: Apache-2.0
#include "app/manifest.h"


#include <app/package_manifest.h>
#include <app/private/package_manifest_parsing.h>

#include <charconv>

#include <tactility/log.h>

constexpr auto* TAG = "app_metadata_v2";

error_t package_manifest_parse_v2(const std::map<std::string, std::string>& properties, PackageManifest& out_package, AppManifestBinding* out_bindings, size_t bindings_capacity) {
    // manifest

    std::string format_version;
    if (!app_package_manifest_get_value(properties, "manifest.version", format_version)) {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!app_package_manifest_is_valid_format_version(format_version)) {
        LOG_E(TAG, "Invalid version");
        return ERROR_INVALID_ARGUMENT;
    }

    // app

    std::string id;
    if (!app_package_manifest_get_value(properties, "app.id", id)) {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!app_manifest_id_is_valid(id.c_str())) {
        LOG_E(TAG, "Invalid app id");
        return ERROR_INVALID_ARGUMENT;
    }

    if (!app_package_manifest_copy_bounded(out_package.id, sizeof(out_package.id), id)) {
        LOG_E(TAG, "App id too long");
        return ERROR_INVALID_ARGUMENT;
    }

    std::string name;
    if (!app_package_manifest_get_value(properties, "app.name", name)) {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!app_manifest_name_is_valid(name.c_str())) {
        LOG_E(TAG, "Invalid app name");
        return ERROR_INVALID_ARGUMENT;
    }

    std::string version_name;
    if (!app_package_manifest_get_value(properties, "app.version.name", version_name)) {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!app_package_manifest_is_valid_version_name(version_name)) {
        LOG_E(TAG, "Invalid app version name");
        return ERROR_INVALID_ARGUMENT;
    }

    if (!app_package_manifest_copy_bounded(out_package.version_name, sizeof(out_package.version_name), version_name)) {
        LOG_E(TAG, "App version name too long");
        return ERROR_INVALID_ARGUMENT;
    }

    std::string version_code_string;
    if (!app_package_manifest_get_value(properties, "app.version.code", version_code_string)) {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!app_package_manifest_is_valid_version_code(version_code_string)) {
        LOG_E(TAG, "Invalid app version code");
        return ERROR_INVALID_ARGUMENT;
    }

    uint64_t version_code = 0;
    const auto* first = version_code_string.data();
    const auto* last = first + version_code_string.size();
    if (std::from_chars(first, last, version_code).ec != std::errc {}) {
        LOG_E(TAG, "App version code out of range");
        return ERROR_INVALID_ARGUMENT;
    }
    out_package.version_code = version_code;

    // target

    std::string target_sdk;
    if (!app_package_manifest_get_value(properties, "target.sdk", target_sdk)) {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!app_package_manifest_copy_bounded(out_package.target_sdk, sizeof(out_package.target_sdk), target_sdk)) {
        LOG_E(TAG, "Target sdk too long");
        return ERROR_INVALID_ARGUMENT;
    }

    // requires.device.id (optional; if present, must be a non-empty comma-separated list)

    auto device_id_iterator = properties.find("requires.device.id");
    if (device_id_iterator != properties.end()) {
        const std::string& device_id = device_id_iterator->second;
        if (!app_package_manifest_is_valid_device_id_list(device_id)) {
            LOG_E(TAG, "Invalid requires.device.id");
            return ERROR_INVALID_ARGUMENT;
        }
        if (!app_package_manifest_copy_bounded(out_package.requires_device_id, sizeof(out_package.requires_device_id), device_id)) {
            LOG_E(TAG, "requires.device.id too long");
            return ERROR_INVALID_ARGUMENT;
        }
    }

    // A v2 manifest always describes exactly one app, sharing the package's own id.
    out_package.app_manifest_count = 1;
    if (bindings_capacity == 0) {
        return ERROR_NONE;
    }

    AppManifestBinding& binding = out_bindings[0];
    AppManifest& manifest = binding.manifest;

    // v2 predates the "binary" key - the single app always installs as bin/<platform>/app.{elf,so}.
    if (!app_package_manifest_copy_bounded(binding.binary, sizeof(binding.binary), "app")) {
        LOG_E(TAG, "Binary name too long");
        return ERROR_INVALID_ARGUMENT;
    }

    if (!app_package_manifest_copy_bounded(manifest.id, sizeof(manifest.id), id)) {
        LOG_E(TAG, "App id too long");
        return ERROR_INVALID_ARGUMENT;
    }
    if (!app_package_manifest_copy_bounded(manifest.name, sizeof(manifest.name), name)) {
        LOG_E(TAG, "App name too long");
        return ERROR_INVALID_ARGUMENT;
    }

    // app.stack.depth (optional; if present, must be a valid unsigned decimal fitting uint32_t)

    auto stack_size_iterator = properties.find("app.stack.depth");
    if (stack_size_iterator != properties.end()) {
        const std::string& stack_size_string = stack_size_iterator->second;
        if (!app_manifest_stack_size_is_valid(stack_size_string.c_str())) {
            LOG_E(TAG, "Invalid app.stack.depth");
            return ERROR_INVALID_ARGUMENT;
        }

        uint32_t stack_size = 0;
        const auto* stack_size_first = stack_size_string.data();
        const auto* stack_size_last = stack_size_first + stack_size_string.size();
        if (std::from_chars(stack_size_first, stack_size_last, stack_size).ec != std::errc {}) {
            LOG_E(TAG, "App stack depth out of range");
            return ERROR_INVALID_ARGUMENT;
        }

        // Reject outright rather than truncating/clamping into AppStackConfig::depth (uint16_t) -
        // a value like 1073741825 would otherwise silently narrow to 1, handing the app a
        // catastrophically undersized stack instead of the huge one it declared.
        if (stack_size > APP_STACK_SIZE_MAX) {
            LOG_E(TAG, "App stack depth %u exceeds APP_STACK_SIZE_MAX(%u)", stack_size, APP_STACK_SIZE_MAX);
            return ERROR_INVALID_ARGUMENT;
        }
        manifest.stack.depth = static_cast<uint16_t>(stack_size);
    }

    // v2 predates per-app visibility - always visible.
    manifest.flags = 0;
    manifest.category = APP_CATEGORY_USER;

    return ERROR_NONE;
}
