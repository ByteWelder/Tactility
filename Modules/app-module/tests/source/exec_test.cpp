// SPDX-License-Identifier: Apache-2.0
#include "doctest.h"

#include <app/exec.h>
#include <app/loader.h>
#include <app/location.h>

#include <service/instance.h>
#include <service/manager.h>

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

extern ServiceManifest app_internal_loader_service_manifest;

namespace {

const AppLoaderApi* memory_loader_api() {
    if (service_manager_find_instance(APP_LOADER_MEMORY_SERVICE_ID) == nullptr) {
        service_manager_add(&app_internal_loader_service_manifest, /*auto_start=*/true);
    }
    auto* instance = service_manager_find_instance(APP_LOADER_MEMORY_SERVICE_ID);
    REQUIRE(instance != nullptr);
    return static_cast<const AppLoaderApi*>(service_instance_get_data(instance));
}

int32_t fake_entry(int, char*[]) {
    return 0;
}

// Creates <tmp>/{bin,outside} plus a real file at outside/target.so, for the symlink-escape
// tests below. Caller removes everything via remove_symlink_fixture() once done.
struct SymlinkFixture {
    std::string root;
    std::string bin_dir;
    std::string outside_dir;
    std::string outside_target;

    bool create() {
        char temp_template[] = "/tmp/exec-test-symlink-XXXXXX";
        char* temp_dir = mkdtemp(temp_template);
        if (temp_dir == nullptr) {
            return false;
        }
        root = temp_dir;
        bin_dir = root + "/bin";
        outside_dir = root + "/outside";
        outside_target = outside_dir + "/target.so";

        if (mkdir(bin_dir.c_str(), 0755) != 0 || mkdir(outside_dir.c_str(), 0755) != 0) {
            return false;
        }
        FILE* file = fopen(outside_target.c_str(), "w");
        if (file == nullptr) {
            return false;
        }
        fclose(file);
        return true;
    }
};

void remove_symlink_fixture(const SymlinkFixture& fixture) {
    unlink((fixture.bin_dir + "/escape.so").c_str());
    unlink((fixture.bin_dir + "/inside.so").c_str());
    unlink((fixture.bin_dir + "/real.so").c_str());
    unlink(fixture.outside_target.c_str());
    rmdir(fixture.bin_dir.c_str());
    rmdir(fixture.outside_dir.c_str());
    rmdir(fixture.root.c_str());
}

} // namespace

TEST_CASE("app_exec_path_add/app_exec_path_allowed accept nested children of a registered directory") {
    REQUIRE_EQ(app_exec_path_add("/exec-test/bin"), ERROR_NONE);

    CHECK(app_exec_path_allowed("/exec-test/bin/foo"));
    CHECK(app_exec_path_allowed("/exec-test/bin/sub/foo"));
    CHECK_FALSE(app_exec_path_allowed("/exec-test/binaries/foo"));
    CHECK_FALSE(app_exec_path_allowed("/exec-test/bin"));

    app_exec_path_remove("/exec-test/bin");
}

TEST_CASE("app_exec_path_allowed rejects a '..' segment even under a registered directory") {
    REQUIRE_EQ(app_exec_path_add("/exec-test/bin2"), ERROR_NONE);

    CHECK_FALSE(app_exec_path_allowed("/exec-test/bin2/../etc/passwd"));

    app_exec_path_remove("/exec-test/bin2");
}

TEST_CASE("app_exec_path_add is a no-op for a path already registered") {
    REQUIRE_EQ(app_exec_path_add("/exec-test/dup"), ERROR_NONE);
    REQUIRE_EQ(app_exec_path_add("/exec-test/dup"), ERROR_NONE);

    CHECK(app_exec_path_allowed("/exec-test/dup/foo"));

    app_exec_path_remove("/exec-test/dup");
}

TEST_CASE("app_exec_path_add does not register same item twice") {
    REQUIRE_EQ(app_exec_path_add("/exec-test/dup"), ERROR_NONE);
    REQUIRE_EQ(app_exec_path_add("/exec-test/dup"), ERROR_NONE);
    app_exec_path_remove("/exec-test/dup");
    CHECK(!app_exec_path_allowed("/exec-test/dup/foo"));
}

TEST_CASE("app_exec_path_remove reports ERROR_NOT_FOUND for a path that was never registered") {
    CHECK_EQ(app_exec_path_remove("/exec-test/never-registered"), ERROR_NOT_FOUND);
}

TEST_CASE("app_exec_path_remove makes a previously allowed path disallowed again") {
    REQUIRE_EQ(app_exec_path_add("/exec-test/removable"), ERROR_NONE);
    REQUIRE(app_exec_path_allowed("/exec-test/removable/foo"));

    REQUIRE_EQ(app_exec_path_remove("/exec-test/removable"), ERROR_NONE);
    CHECK_FALSE(app_exec_path_allowed("/exec-test/removable/foo"));
}

TEST_CASE("app_exec_path_add resolves a relative path to absolute, so it matches an absolute incoming path") {
    char cwd[PATH_MAX];
    REQUIRE(getcwd(cwd, sizeof(cwd)) != nullptr);

    REQUIRE_EQ(app_exec_path_add("exec-test-relative/bin"), ERROR_NONE);

    std::string absolute_child = std::string(cwd) + "/exec-test-relative/bin/foo";
    CHECK(app_exec_path_allowed(absolute_child.c_str()));

    app_exec_path_remove("exec-test-relative/bin");
    CHECK_FALSE(app_exec_path_allowed(absolute_child.c_str()));
}

TEST_CASE("app_exec_path_allowed resolves a relative incoming path to absolute, so it matches an absolute registered directory") {
    char cwd[PATH_MAX];
    REQUIRE(getcwd(cwd, sizeof(cwd)) != nullptr);

    std::string absolute_dir = std::string(cwd) + "/exec-test-relative2/bin";
    REQUIRE_EQ(app_exec_path_add(absolute_dir.c_str()), ERROR_NONE);

    CHECK(app_exec_path_allowed("exec-test-relative2/bin/foo"));

    app_exec_path_remove(absolute_dir.c_str());
}

TEST_CASE("app_exec_path_allowed rejects a symlink inside a registered directory that resolves outside it") {
    SymlinkFixture fixture;
    REQUIRE(fixture.create());

    std::string escape_link = fixture.bin_dir + "/escape.so";
    REQUIRE_EQ(symlink(fixture.outside_target.c_str(), escape_link.c_str()), 0);

    REQUIRE_EQ(app_exec_path_add(fixture.bin_dir.c_str()), ERROR_NONE);

    // Lexically "escape.so" is inside bin_dir, but it physically resolves to outside_dir. Must
    // be rejected, or the posix loader would dlopen() a binary the caller never allowed.
    CHECK_FALSE(app_exec_path_allowed(escape_link.c_str()));

    app_exec_path_remove(fixture.bin_dir.c_str());
    remove_symlink_fixture(fixture);
}

TEST_CASE("app_exec_path_allowed accepts a real file, and a symlink that stays inside, a registered directory") {
    SymlinkFixture fixture;
    REQUIRE(fixture.create());

    std::string real_file = fixture.bin_dir + "/real.so";
    FILE* file = fopen(real_file.c_str(), "w");
    REQUIRE(file != nullptr);
    fclose(file);

    std::string inside_link = fixture.bin_dir + "/inside.so";
    REQUIRE_EQ(symlink(real_file.c_str(), inside_link.c_str()), 0);

    REQUIRE_EQ(app_exec_path_add(fixture.bin_dir.c_str()), ERROR_NONE);

    CHECK(app_exec_path_allowed(real_file.c_str()));
    CHECK(app_exec_path_allowed(inside_link.c_str()));

    app_exec_path_remove(fixture.bin_dir.c_str());
    remove_symlink_fixture(fixture);
}

TEST_CASE("app_exec_path_add returns ERROR_OUT_OF_MEMORY once the registry is full") {
    char path[32];
    int added = 0;
    for (added = 0; added < APP_EXEC_MAX_PATHS + 1; added++) {
        snprintf(path, sizeof(path), "/exec-test/overflow%d", added);
        if (app_exec_path_add(path) != ERROR_NONE) {
            break;
        }
    }
    CHECK_LT(added, APP_EXEC_MAX_PATHS + 1);

    for (int i = 0; i < added; i++) {
        snprintf(path, sizeof(path), "/exec-test/overflow%d", i);
        app_exec_path_remove(path);
    }
}

TEST_CASE("memory loader's is_executable() reports a non-null AppLocation as executable, a null one as not") {
    const AppLoaderApi* loader = memory_loader_api();
    REQUIRE(loader->is_executable != nullptr);

    AppLocation runnable { APP_LOCATION_MEMORY, reinterpret_cast<void*>(fake_entry) };
    CHECK(loader->is_executable(runnable));

    AppLocation empty { APP_LOCATION_MEMORY, nullptr };
    CHECK_FALSE(loader->is_executable(empty));

    AppLocation wrong_type { APP_LOCATION_PATH, reinterpret_cast<void*>(fake_entry) };
    CHECK_FALSE(loader->is_executable(wrong_type));
}
