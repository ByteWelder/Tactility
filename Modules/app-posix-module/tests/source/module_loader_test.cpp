// SPDX-License-Identifier: Apache-2.0
#include "doctest.h"

#include <app/install.h>
#include <app/loader.h>
#include <app/private/module_loader.h>

#include <service/manager.h>

#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/module.h>

#include <atomic>
#include <string>
#include <vector>

extern ServiceManifest loader_service_manifest; // app-posix-module's own

namespace {

std::atomic<int> g_starts { 0 };
std::atomic<int> g_stops { 0 };

const AppLoaderApi& path_loader() {
    if (service_manager_find_instance(APP_LOADER_PATH_SERVICE_ID) == nullptr) {
        service_manager_add(&loader_service_manifest, /*auto_start=*/true);
    }
    ServiceInstance* instance = service_manager_find_instance(APP_LOADER_PATH_SERVICE_ID);
    return *static_cast<const AppLoaderApi*>(service_instance_get_data(instance));
}

// Keeps the installed fixture loaded so its static Module stays valid. dlopen() yields the same instance for every load of one file.
Module* hold_fixture(AppRuntime* out_runtime) {
    const auto& loader = path_loader();
    AppLocation location { APP_LOCATION_PATH, const_cast<char*>(MODULE_INSTALL_DIR_BINARY_PATH) };
    REQUIRE_EQ(loader.load(location, out_runtime), ERROR_NONE);
    Module* module = nullptr;
    REQUIRE_EQ(loader.get_module(*out_runtime, &module), ERROR_NONE);
    return module;
}

} // namespace

// Called by the dlopen()ed module fixture
extern "C" void module_fixture_event(bool started) {
    (started ? g_starts : g_stops)++;
}

TEST_CASE("loader get_module returns the module main() returned") {
    const auto& loader = path_loader();
    AppLocation location { APP_LOCATION_PATH, const_cast<char*>(MODULE_FIXTURE_PATH) };

    AppRuntime runtime = nullptr;
    REQUIRE_EQ(loader.load(location, &runtime), ERROR_NONE);

    Module* module = nullptr;
    REQUIRE_EQ(loader.get_module(runtime, &module), ERROR_NONE);
    REQUIRE_NE(module, nullptr);
    CHECK_EQ(std::string(module->name), "module-fixture");

    loader.unload(runtime);
}

TEST_CASE("module loader starts the modules of a package and stops them on unload") {
    path_loader();
    AppRuntime holder = nullptr;
    Module* module = hold_fixture(&holder);
    CHECK_FALSE(module_is_external(module));

    const int starts_before = g_starts;
    const int stops_before = g_stops;

    REQUIRE_EQ(app_module_loader_load_package("test.module.package", MODULE_INSTALL_DIR_PATH, { "module_fixture" }), ERROR_NONE);
    CHECK_EQ(g_starts - starts_before, 1);
    CHECK_EQ(g_stops - stops_before, 0);

    CHECK(module_is_external(module));

    CHECK_EQ(app_module_loader_load_package("test.module.package", MODULE_INSTALL_DIR_PATH, { "module_fixture" }), ERROR_INVALID_STATE);

    app_module_loader_unload_package("test.module.package");
    CHECK_EQ(g_stops - stops_before, 1);
    CHECK_FALSE(module_is_external(module));

    // Unloading again is a no-op
    app_module_loader_unload_package("test.module.package");
    CHECK_EQ(g_stops - stops_before, 1);

    path_loader().unload(holder);
}

TEST_CASE("module loader keeps a module that is in use loaded") {
    path_loader();
    AppRuntime holder = nullptr;
    Module* module = hold_fixture(&holder);

    const int stops_before = g_stops;
    REQUIRE_EQ(app_module_loader_load_package("test.module.busy", MODULE_INSTALL_DIR_PATH, { "module_fixture" }), ERROR_NONE);

    static Device device = { .name = "module_fixture_device", .config = nullptr, .parent = nullptr };
    REQUIRE_EQ(device_construct(&device), ERROR_NONE);
    REQUIRE_EQ(device_add(&device), ERROR_NONE);
    REQUIRE_EQ(driver_bind(module->drivers[0], &device), ERROR_NONE);

    // Refused, and nothing was stopped or unloaded
    CHECK_EQ(app_module_loader_unload_package("test.module.busy"), ERROR_RESOURCE_BUSY);
    CHECK_EQ(g_stops - stops_before, 0);
    CHECK(module_is_started(module));
    int listed = 0;
    app_module_for_each([](const AppModuleInfo*, void* context) { (*static_cast<int*>(context))++; }, &listed);
    CHECK_EQ(listed, 1);

    REQUIRE_EQ(driver_unbind(module->drivers[0], &device), ERROR_NONE);
    CHECK_EQ(app_module_loader_unload_package("test.module.busy"), ERROR_NONE);
    CHECK_EQ(g_stops - stops_before, 1);

    CHECK_EQ(device_remove(&device), ERROR_NONE);
    CHECK_EQ(device_destruct(&device), ERROR_NONE);
    path_loader().unload(holder);
}

TEST_CASE("module loader leaves nothing running when a binary is missing") {
    path_loader();
    const int starts_before = g_starts;
    const int stops_before = g_stops;

    CHECK_NE(app_module_loader_load_package("test.module.partial", MODULE_INSTALL_DIR_PATH, { "module_fixture", "missing" }), ERROR_NONE);
    CHECK_EQ(g_starts - starts_before, 1);
    CHECK_EQ(g_stops - stops_before, 1);

    // The failed attempt must not have left the package registered
    CHECK_EQ(app_module_loader_load_package("test.module.partial", MODULE_INSTALL_DIR_PATH, { "module_fixture" }), ERROR_NONE);
    app_module_loader_unload_package("test.module.partial");
}

TEST_CASE("app_module_for_each visits the modules of loaded packages only") {
    path_loader();
    struct Visit {
        std::string package_id;
        std::string install_dir;
        Module* module = nullptr;
    };
    auto collect = [](const AppModuleInfo* info, void* context) {
        static_cast<std::vector<Visit>*>(context)->push_back({ info->package_id, info->install_dir, info->module });
    };

    std::vector<Visit> before;
    app_module_for_each(collect, &before);
    CHECK(before.empty());

    AppRuntime holder = nullptr;
    Module* module = hold_fixture(&holder);
    REQUIRE_EQ(app_module_loader_load_package("test.module.listed", MODULE_INSTALL_DIR_PATH, { "module_fixture" }), ERROR_NONE);

    std::vector<Visit> during;
    app_module_for_each(collect, &during);
    REQUIRE_EQ(during.size(), 1);
    CHECK_EQ(during[0].package_id, "test.module.listed");
    CHECK_EQ(during[0].install_dir, MODULE_INSTALL_DIR_PATH);
    CHECK_EQ(during[0].module, module);

    app_module_loader_unload_package("test.module.listed");
    std::vector<Visit> after;
    app_module_for_each(collect, &after);
    CHECK(after.empty());

    path_loader().unload(holder);
}

TEST_CASE("app_module_set_started starts and stops a loaded module by name") {
    path_loader();
    const int starts_before = g_starts;
    const int stops_before = g_stops;

    REQUIRE_EQ(app_module_loader_load_package("test.module.toggle", MODULE_INSTALL_DIR_PATH, { "module_fixture" }), ERROR_NONE);
    CHECK_EQ(g_starts - starts_before, 1);

    CHECK_EQ(app_module_set_started("test.module.toggle", "module-fixture", false), ERROR_NONE);
    CHECK_EQ(g_stops - stops_before, 1);
    // Stopping a stopped module does nothing
    CHECK_EQ(app_module_set_started("test.module.toggle", "module-fixture", false), ERROR_NONE);
    CHECK_EQ(g_stops - stops_before, 1);

    CHECK_EQ(app_module_set_started("test.module.toggle", "module-fixture", true), ERROR_NONE);
    CHECK_EQ(g_starts - starts_before, 2);
    CHECK_EQ(app_module_set_started("test.module.toggle", "module-fixture", true), ERROR_NONE);
    CHECK_EQ(g_starts - starts_before, 2);

    CHECK_EQ(app_module_set_started("test.module.toggle", "unknown", true), ERROR_NOT_FOUND);
    CHECK_EQ(app_module_set_started("unknown.package", "module-fixture", true), ERROR_NOT_FOUND);

    app_module_loader_unload_package("test.module.toggle");
    // Once unloaded, the module can't be reached anymore
    CHECK_EQ(app_module_set_started("test.module.toggle", "module-fixture", true), ERROR_NOT_FOUND);
}
