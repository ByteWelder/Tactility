// SPDX-License-Identifier: Apache-2.0
#include "doctest.h"

#include <app/event.h>
#include <app/exec.h>
#include <app/loader.h>
#include <app/manager.h>
#include <app/scheduler.h>

#include <service/manager.h>

#include <tactility/delay.h>

#include <atomic>
#include <string>

extern ServiceManifest loader_service_manifest;              // app-posix-module's own
extern ServiceManifest app_internal_loader_service_manifest; // app-module's real memory loader

namespace {

void ensure_path_loader_registered() {
    if (service_manager_find_instance(APP_LOADER_PATH_SERVICE_ID) == nullptr) {
        service_manager_add(&loader_service_manifest, /*auto_start=*/true);
    }
}

void ensure_memory_loader_registered() {
    if (service_manager_find_instance(APP_LOADER_MEMORY_SERVICE_ID) == nullptr) {
        service_manager_add(&app_internal_loader_service_manifest, /*auto_start=*/true);
    }
}

std::string directory_of(const std::string& path) {
    auto slash = path.find_last_of('/');
    return slash == std::string::npos ? "." : path.substr(0, slash);
}

const std::string FIXTURE_DIR = directory_of(FIXTURE_APP_PATH);

bool wait_for_state(AppInstanceId id, AppInstanceState target, uint32_t timeout_ms) {
    uint32_t waited = 0;
    while (waited < timeout_ms) {
        if (app_manager_get_state(id) == target) {
            return true;
        }
        delay_millis(10);
        waited += 10;
    }
    return app_manager_get_state(id) == target;
}

std::atomic<int32_t> g_fixture_result { -1 };
std::atomic<bool> g_fixture_result_received { false };

// Starts the dlopen()ed fixture as its own modal child and stashes its returned result, so the
// test can inspect that result from the (in-process, directly readable) parent's own task.
int32_t parent_app_main(int, char*[]) {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    AppEventSubscription sub {};
    app_event_subscribe(&sub, &event_group);

    AppInstanceId self_id = app_scheduler_current_app_id();

    AppManifest fixture_manifest {
        "test.posix.fixture", "Fixture", APP_CATEGORY_USER,
        { APP_LOCATION_PATH, const_cast<char*>(FIXTURE_APP_PATH) }
    };
    app_manager_add(&fixture_manifest);

    AppInstanceId fixture_id = 0;
    app_manager_start_for_result("test.posix.fixture", self_id, 0, nullptr, &fixture_id);

    while (true) {
        if (task_event_group_wait_any(&event_group, nullptr, pdMS_TO_TICKS(5000)) != ERROR_NONE) {
            break; // safety net so a bug here can't hang the test suite
        }
        AppEvent event {};
        bool got_result = false;
        while (app_event_poll(&sub, &event) == ERROR_NONE) {
            if (event.type == APP_EVENT_RESULT && event.result.launch_id == fixture_id) {
                g_fixture_result.store(event.result.result, std::memory_order_release);
                got_result = true;
            }
        }
        if (got_result) {
            break;
        }
    }
    g_fixture_result_received.store(true, std::memory_order_release);

    app_manager_remove("test.posix.fixture");
    app_event_unsubscribe(&sub);
    task_event_group_destruct(&event_group);
    return 0;
}

} // namespace

TEST_CASE("app-posix-module's loader-path service dlopen()s a .so and calls its main(), which resolves a real Tactility symbol against the host") {
    ensure_path_loader_registered();
    ensure_memory_loader_registered();
    REQUIRE_EQ(app_exec_path_add(FIXTURE_DIR.c_str()), ERROR_NONE);
    g_fixture_result.store(-1, std::memory_order_relaxed);
    g_fixture_result_received.store(false, std::memory_order_relaxed);

    AppManifest parent_manifest { "test.posix.parent", "Parent", APP_CATEGORY_USER, { APP_LOCATION_MEMORY, reinterpret_cast<void*>(parent_app_main) } };
    REQUIRE_EQ(app_manager_add(&parent_manifest), ERROR_NONE);

    AppInstanceId parent_id = 0;
    REQUIRE_EQ(app_manager_start("test.posix.parent", &parent_id), ERROR_NONE);
    REQUIRE(wait_for_state(parent_id, APP_INSTANCE_STATE_STOPPED, 3000));

    CHECK(g_fixture_result_received.load(std::memory_order_acquire));
    // A positive AppInstanceId proves the fixture's dlopen()ed main() actually resolved and
    // called app_scheduler_current_app_id() against the host process, not just "ran and returned
    // a hardcoded value" - 0 would mean it thought it wasn't running as an app instance at all.
    CHECK_GT(g_fixture_result.load(std::memory_order_acquire), 0);

    app_manager_remove("test.posix.parent");
}

TEST_CASE("app_exec_is_executable_path() accepts a real .so in a registered directory") {
    ensure_path_loader_registered();
    REQUIRE_EQ(app_exec_path_add(FIXTURE_DIR.c_str()), ERROR_NONE);

    CHECK(app_exec_is_executable_path(FIXTURE_APP_PATH));
}

TEST_CASE("app_exec_is_executable_path() rejects the same .so when its directory isn't registered") {
    ensure_path_loader_registered();
    app_exec_path_remove(FIXTURE_DIR.c_str());

    CHECK_FALSE(app_exec_is_executable_path(FIXTURE_APP_PATH));

    // Restore for any later test case that relies on it being registered.
    app_exec_path_add(FIXTURE_DIR.c_str());
}

TEST_CASE("app_exec_is_executable_path() rejects a .so-named file with no ELF header") {
    ensure_path_loader_registered();
    auto non_elf_dir = directory_of(FIXTURE_NON_ELF_PATH);
    REQUIRE_EQ(app_exec_path_add(non_elf_dir.c_str()), ERROR_NONE);

    CHECK_FALSE(app_exec_is_executable_path(FIXTURE_NON_ELF_PATH));

    app_exec_path_remove(non_elf_dir.c_str());
}

TEST_CASE("app_exec_is_executable_path() rejects a nonexistent path in a registered directory") {
    ensure_path_loader_registered();
    REQUIRE_EQ(app_exec_path_add(FIXTURE_DIR.c_str()), ERROR_NONE);

    CHECK_FALSE(app_exec_is_executable_path((FIXTURE_DIR + "/does-not-exist.so").c_str()));
}

TEST_CASE("app_exec_is_executable_path() rejects an install-directory-shaped path missing its per-arch .so") {
    ensure_path_loader_registered();
    REQUIRE_EQ(app_exec_path_add(FIXTURE_DIR.c_str()), ERROR_NONE);

    // FIXTURE_DIR itself has no elf/posix-<arch>.so under it, so resolution fails.
    CHECK_FALSE(app_exec_is_executable_path(FIXTURE_DIR.c_str()));
}
