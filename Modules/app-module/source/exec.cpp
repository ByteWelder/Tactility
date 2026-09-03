// SPDX-License-Identifier: Apache-2.0
#include <app/exec.h>

#include <app/private/exec_internal.h>

#include <app/loader.h>
#include <app/location.h>

#include <service/instance.h>
#include <service/manager.h>

#include <tactility/concurrent/mutex.h>
#include <tactility/filesystem/file_system.h>
#include <tactility/log.h>
#include <tactility/paths.h>

#include <climits>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>

constexpr auto* TAG = "app_exec";

namespace {

struct ExecPathRegistry {
    std::string paths[APP_EXEC_MAX_PATHS];
    size_t count = 0;
    Mutex mutex {};

    ExecPathRegistry() { mutex_construct(&mutex); }
};

ExecPathRegistry& registry() {
    static ExecPathRegistry instance;
    return instance;
}

// POSIX works with relative paths. The rest of the OS uses anchors relative paths.
// Resolving through getcwd() keeps every path callers pass in comparable, regardless of which convention they used.
// A path already starting with '/' passes through unchanged, so this is a no-op on ESP32.
std::string make_absolute(const std::string& path) {
    if (path.empty() || path.front() == '/') {
        return path;
    }
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        return path; // best effort: falls back to today's (broken but not worse) behavior
    }
    return std::string(cwd) + "/" + path;
}

// Resolves to absolute (see make_absolute()) and strips a trailing '/' so "/data/bin/" and
// "/data/bin" register/compare identically.
std::string normalize(const char* path) {
    std::string result = make_absolute(path);
    while (result.size() > 1 && result.back() == '/') {
        result.pop_back();
    }
    return result;
}

bool has_dotdot_segment(const std::string& path) {
    size_t start = 0;
    while (start <= path.size()) {
        size_t slash = path.find('/', start);
        size_t length = (slash == std::string::npos ? path.size() : slash) - start;
        if (path.compare(start, length, "..") == 0) {
            return true;
        }
        if (slash == std::string::npos) {
            break;
        }
        start = slash + 1;
    }
    return false;
}

// Resolves @a absolute_path through every symlink on its way (POSIX realpath()), so a symlink
// sitting inside an allowed directory can't point at a target outside it. is_under_directory()
// only ever sees where a path physically ends up, never the lexical path a caller wrote. Falls
// back to @a absolute_path unresolved if realpath() fails (e.g. nothing exists there yet): a
// nonexistent path can't be dlopen()ed either way, so comparing it unresolved is still safe.
std::string resolve_physical(const std::string& absolute_path) {
    char resolved[PATH_MAX];
    if (realpath(absolute_path.c_str(), resolved) == nullptr) {
        return absolute_path;
    }
    return std::string(resolved);
}

// True when @a file_path lives strictly under @a dir (a direct or nested child). Never equal to
// @a dir itself: a bare directory is never itself an executable candidate.
bool is_under_directory(const std::string& file_path, const std::string& dir) {
    if (file_path.size() <= dir.size() || file_path.compare(0, dir.size(), dir) != 0) {
        return false;
    }
    return file_path[dir.size()] == '/';
}

const AppLoaderApi* find_path_loader_api() {
    ServiceInstance* instance = service_manager_find_instance(APP_LOADER_PATH_SERVICE_ID);
    if (instance == nullptr) {
        return nullptr;
    }
    return static_cast<const AppLoaderApi*>(service_instance_get_data(instance));
}

} // namespace

extern "C" {

error_t app_exec_path_add(const char* path) {
    auto normalized = normalize(path);

    auto& reg = registry();
    mutex_lock(&reg.mutex);

    for (size_t i = 0; i < reg.count; i++) {
        if (reg.paths[i] == normalized) {
            mutex_unlock(&reg.mutex);
            return ERROR_NONE;
        }
    }

    if (reg.count >= APP_EXEC_MAX_PATHS) {
        mutex_unlock(&reg.mutex);
        LOG_E(TAG, "Registry full, can't add %s", path);
        return ERROR_OUT_OF_MEMORY;
    }

    reg.paths[reg.count++] = normalized;
    mutex_unlock(&reg.mutex);
    return ERROR_NONE;
}

error_t app_exec_path_remove(const char* path) {
    auto normalized = normalize(path);

    auto& reg = registry();
    mutex_lock(&reg.mutex);

    for (size_t i = 0; i < reg.count; i++) {
        if (reg.paths[i] == normalized) {
            reg.paths[i] = reg.paths[reg.count - 1];
            reg.count--;
            mutex_unlock(&reg.mutex);
            return ERROR_NONE;
        }
    }

    mutex_unlock(&reg.mutex);
    return ERROR_NOT_FOUND;
}

bool app_exec_path_allowed(const char* file_path) {
    std::string path = make_absolute(file_path);
    if (has_dotdot_segment(path)) {
        return false;
    }
    std::string resolved_path = resolve_physical(path);

    auto& reg = registry();
    mutex_lock(&reg.mutex);
    bool allowed = false;
    for (size_t i = 0; i < reg.count; i++) {
        // Resolved at check time, not at app_exec_path_add() time: the registered directory
        // itself may not exist yet when it's registered (e.g. before the first app_install()).
        std::string resolved_dir = resolve_physical(reg.paths[i]);
        if (is_under_directory(resolved_path, resolved_dir)) {
            allowed = true;
            break;
        }
    }
    mutex_unlock(&reg.mutex);
    return allowed;
}

bool app_exec_is_executable_path(const char* path) {
    const AppLoaderApi* loader = find_path_loader_api();
    if (loader == nullptr || loader->is_executable == nullptr) {
        return false;
    }

    AppLocation location { APP_LOCATION_PATH, const_cast<char*>(path) };
    return loader->is_executable(location);
}

void app_exec_register_default_paths() {
    char data_root[192];
    if (paths_get_data_path(data_root, sizeof(data_root)) == ERROR_NONE) {
        app_exec_path_add((std::string(data_root) + "/bin").c_str());
        app_exec_path_add((std::string(data_root) + "/app").c_str());
    } else {
        LOG_W(TAG, "Failed to resolve data path, not registering its executable directories");
    }
}

} // extern "C"
