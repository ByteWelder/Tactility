// SPDX-License-Identifier: Apache-2.0
#include <app_hub/app_hub.h>
#include <app_hub/app_hub_catalog.h>
#include <app_hub/app_hub_service.h>

#include <app/install.h>
#include <app/manager.h>
#include <app/manifest.h>

#include <http/download.h>

#include <service/paths.h>

#include <tactility/concurrent/mutex.h>
#include <tactility/concurrent/thread.h>
#include <tactility/filesystem/fs.h>
#include <tactility/log.h>
#include <tactility/paths.h>
#include <tactility/time.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr auto* TAG = "app_hub";
constexpr auto* CATALOG_FILE_NAME = "apps.json";
constexpr auto* CATALOG_DOWNLOAD_FILE_NAME = "apps.json.tmp";
constexpr size_t WORKER_STACK_SIZE = 8192;
constexpr TickType_t WORKER_IDLE_TIMEOUT = pdMS_TO_TICKS(5000);

struct Operation {
    uint32_t id;
    AppHubOperationType type;
    PackageId app_id;
};

struct InstalledPackage {
    PackageManifest package;
    AppId first_app_id;
};

using AppList = std::vector<AppHubApp, tt::OptExternalAllocator<AppHubApp>>;

struct GlobalMutex {
    Mutex handle {};
    GlobalMutex() { mutex_construct(&handle); }
    ~GlobalMutex() { mutex_destruct(&handle); }
};

GlobalMutex globalMutex;

void lock() { mutex_lock(&globalMutex.handle); }
void unlock() { mutex_unlock(&globalMutex.handle); }

// region State, guarded by the global lock

AppHubCatalog catalog;
bool catalogLoaded = false;
std::deque<Operation> queue;
Operation runningOperation {};
bool operationRunning = false;
uint32_t nextOperationId = 1;
AppHubEventSubscription* subscriptions = nullptr;

bool serviceRunning = false;
bool stopping = false;
Thread* worker = nullptr;
bool workerRunning = false;
TaskEventGroup workerEvents {};
uint32_t queueBit = 0;
uint32_t stopBit = 0;

// endregion

// region Events

AppHubEventType getStartedEventType(AppHubOperationType operation) {
    switch (operation) {
        case APP_HUB_OPERATION_REFRESH: return APP_HUB_EVENT_REFRESHING;
        case APP_HUB_OPERATION_INSTALL: return APP_HUB_EVENT_INSTALLING;
        case APP_HUB_OPERATION_REMOVE: return APP_HUB_EVENT_REMOVING;
        case APP_HUB_OPERATION_UPDATE_ALL: return APP_HUB_EVENT_UPDATING;
    }
    return APP_HUB_EVENT_REFRESHING;
}

AppHubEventType getFinishedEventType(AppHubOperationType operation) {
    switch (operation) {
        case APP_HUB_OPERATION_REFRESH: return APP_HUB_EVENT_REFRESHED;
        case APP_HUB_OPERATION_INSTALL: return APP_HUB_EVENT_INSTALLED;
        case APP_HUB_OPERATION_REMOVE: return APP_HUB_EVENT_REMOVED;
        case APP_HUB_OPERATION_UPDATE_ALL: return APP_HUB_EVENT_UPDATED;
    }
    return APP_HUB_EVENT_REFRESHED;
}

/**
 * Requires the global lock
 * @param[in] appId the app the event is about, which an update of all apps reports per app
 */
void emitEvent(AppHubEventType type, const Operation& operation, const char* appId, error_t result) {
    AppHubEvent event {};
    event.type = type;
    event.operation_id = operation.id;
    strcpy(event.app_id, appId);
    event.result = result;
    for (auto* sub = subscriptions; sub != nullptr; sub = sub->internal.next) {
        if (sub->internal.count >= APP_HUB_EVENT_QUEUE_CAPACITY) {
            LOG_W(TAG, "A subscriber's event queue is full: dropped an event of operation %lu", static_cast<unsigned long>(event.operation_id));
            continue;
        }
        const uint8_t tail = (sub->internal.head + sub->internal.count) % APP_HUB_EVENT_QUEUE_CAPACITY;
        sub->internal.queue[tail] = event;
        sub->internal.count++;
        task_event_group_signal(sub->internal.event_group, sub->bit);
    }
}

// endregion

// region Catalog

bool getCatalogPath(const char* fileName, char* path, size_t pathSize) {
    return service_paths_get_user_data_path(APP_HUB_SERVICE_ID, fileName, path, pathSize) == ERROR_NONE;
}

/** Requires the global lock */
void ensureCatalogLoaded() {
    if (catalogLoaded) {
        return;
    }
    char path[FILE_MAX_PATH_STRING_LENGTH];
    if (getCatalogPath(CATALOG_FILE_NAME, path, sizeof(path)) && path_exists(path)) {
        catalogLoaded = app_hub_catalog_parse(path, catalog);
    }
}

/** Requires the global lock */
const AppHubCatalogEntry* findCatalogEntry(const char* id) {
    ensureCatalogLoaded();
    for (const auto& entry : catalog) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

// endregion

// region Apps

void copyString(char* target, size_t targetSize, const char* source) {
    strncpy(target, source, targetSize - 1);
    target[targetSize - 1] = '\0';
}

/** Copies the installed packages, as app_manager_for_each_package() holds the app manager's lock */
std::vector<InstalledPackage> getInstalledPackages() {
    std::vector<InstalledPackage> packages;
    app_manager_for_each_package([](const AppPackage* package, void* context) {
        auto& packages = *static_cast<std::vector<InstalledPackage>*>(context);
        InstalledPackage installed {};
        installed.package = package->package;
        if (package->app_id_count > 0) {
            copyString(installed.first_app_id, sizeof(installed.first_app_id), package->app_ids[0]);
        }
        packages.push_back(installed);
    }, &packages);
    return packages;
}

/** Requires the global lock. Only one refresh and one install or remove can be queued or running at a time. */
const Operation* findPendingOperationOfKind(bool refresh) {
    if (operationRunning && (runningOperation.type == APP_HUB_OPERATION_REFRESH) == refresh) {
        return &runningOperation;
    }
    for (const auto& operation : queue) {
        if ((operation.type == APP_HUB_OPERATION_REFRESH) == refresh) {
            return &operation;
        }
    }
    return nullptr;
}

/** Requires the global lock */
const Operation* findPendingOperation(const char* id) {
    if (operationRunning && runningOperation.type != APP_HUB_OPERATION_REFRESH && strcmp(runningOperation.app_id, id) == 0) {
        return &runningOperation;
    }
    for (const auto& operation : queue) {
        if (operation.type != APP_HUB_OPERATION_REFRESH && strcmp(operation.app_id, id) == 0) {
            return &operation;
        }
    }
    return nullptr;
}

/** Requires the global lock */
AppList collectApps(const std::vector<InstalledPackage>& installedPackages) {
    ensureCatalogLoaded();
    AppList apps;
    for (const auto& entry : catalog) {
        if (!app_hub_catalog_is_compatible(entry)) {
            continue;
        }
        AppHubApp app {};
        copyString(app.id, sizeof(app.id), entry.id.c_str());
        copyString(app.name, sizeof(app.name), entry.name.c_str());
        copyString(app.version_name, sizeof(app.version_name), entry.version_name.c_str());
        app.version_code = static_cast<uint64_t>(entry.version_code);
        app.in_catalog = true;
        apps.push_back(app);
    }

    for (const auto& installed : installedPackages) {
        auto existing = std::ranges::find_if(apps, [&installed](const AppHubApp& app) {
            return strcmp(app.id, installed.package.id) == 0;
        });
        AppHubApp* app;
        if (existing != apps.end()) {
            app = &*existing;
        } else {
            app = &apps.emplace_back();
            copyString(app->id, sizeof(app->id), installed.package.id);
            AppManifest manifest;
            const bool hasName = installed.first_app_id[0] != '\0' && app_manager_find_manifest(installed.first_app_id, &manifest) == ERROR_NONE;
            copyString(app->name, sizeof(app->name), hasName ? manifest.name : installed.package.id);
        }
        app->installed = true;
        copyString(app->installed_version_name, sizeof(app->installed_version_name), installed.package.version_name);
        app->installed_version_code = installed.package.version_code;
        app->update_available = app->in_catalog && app->installed_version_code < app->version_code;
    }

    const Operation* pendingAppOperation = findPendingOperationOfKind(false);
    const bool updatingAll = pendingAppOperation != nullptr && pendingAppOperation->type == APP_HUB_OPERATION_UPDATE_ALL;
    for (auto& app : apps) {
        const Operation* pending = findPendingOperation(app.id);
        if (pending == nullptr && updatingAll && app.update_available) {
            pending = pendingAppOperation;
        }
        app.operation_pending = pending != nullptr;
        if (pending != nullptr) {
            app.pending_operation = pending->type;
        }
    }
    return apps;
}

bool containsIgnoringCase(const char* text, const char* part) {
    const std::string_view textView(text);
    const std::string_view partView(part);
    return std::ranges::search(textView, partView, [](char left, char right) {
        return std::tolower(static_cast<unsigned char>(left)) == std::tolower(static_cast<unsigned char>(right));
    }).begin() != textView.end() || partView.empty();
}

bool lessIgnoringCase(const AppHubApp& left, const AppHubApp& right) {
    return std::ranges::lexicographical_compare(std::string_view(left.name), std::string_view(right.name), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) < std::tolower(static_cast<unsigned char>(b));
    });
}

// endregion

// region Operations

error_t download(const char* url, const char* targetPath) {
    HttpDownloadSubscription sub {};
    if (http_download_subscribe(&sub, &workerEvents) != ERROR_NONE) {
        return ERROR_RESOURCE;
    }
    if (http_download_start(url, app_hub_catalog_get_certificate_path().c_str(), targetPath, &sub) != ERROR_NONE) {
        http_download_unsubscribe(&sub);
        return ERROR_RESOURCE;
    }

    while (true) {
        uint32_t flags = 0;
        task_event_group_wait(&workerEvents, sub.bit | stopBit, false, &flags, portMAX_DELAY);
        if (flags & stopBit) {
            http_download_cancel(&sub);
            http_download_unsubscribe(&sub);
            return ERROR_INVALID_STATE;
        }
        HttpDownloadEvent event {};
        if (http_download_poll(&sub, &event) == ERROR_NONE) {
            http_download_unsubscribe(&sub);
            if (event.type != HTTP_DOWNLOAD_EVENT_SUCCESS) {
                LOG_E(TAG, "Download of %s failed (status %d)", url, event.status_code);
                return ERROR_RESOURCE;
            }
            return ERROR_NONE;
        }
    }
}

error_t ensureDirectory(const char* path) {
    const error_t error = directory_make(path, true);
    return (error == ERROR_ALREADY_EXISTS) ? ERROR_NONE : error;
}

error_t executeRefresh() {
    char directory[FILE_MAX_PATH_STRING_LENGTH];
    char downloadPath[FILE_MAX_PATH_STRING_LENGTH];
    char catalogPath[FILE_MAX_PATH_STRING_LENGTH];
    if (service_paths_get_user_data_directory(APP_HUB_SERVICE_ID, directory, sizeof(directory)) != ERROR_NONE ||
        !getCatalogPath(CATALOG_DOWNLOAD_FILE_NAME, downloadPath, sizeof(downloadPath)) ||
        !getCatalogPath(CATALOG_FILE_NAME, catalogPath, sizeof(catalogPath))) {
        return ERROR_NOT_FOUND;
    }
    error_t error = ensureDirectory(directory);
    if (error != ERROR_NONE) {
        return error;
    }

    error = download(app_hub_catalog_get_url().c_str(), downloadPath);
    if (error != ERROR_NONE) {
        file_remove(downloadPath);
        return error;
    }

    AppHubCatalog downloaded;
    if (!app_hub_catalog_parse(downloadPath, downloaded)) {
        file_remove(downloadPath);
        return ERROR_INVALID_ARGUMENT;
    }
    error = file_move(downloadPath, catalogPath, true);
    if (error != ERROR_NONE) {
        return error;
    }

    lock();
    catalog = std::move(downloaded);
    catalogLoaded = true;
    unlock();
    return ERROR_NONE;
}

error_t executeInstall(const char* id) {
    lock();
    const AppHubCatalogEntry* entry = findCatalogEntry(id);
    const bool compatible = entry != nullptr && app_hub_catalog_is_compatible(*entry);
    const std::string file = (entry != nullptr) ? entry->file : "";
    unlock();
    if (entry == nullptr) {
        return ERROR_NOT_FOUND;
    }
    if (!compatible) {
        return ERROR_NOT_SUPPORTED;
    }

    char tempDirectory[FILE_MAX_PATH_STRING_LENGTH];
    error_t error = paths_get_temp_path(tempDirectory, sizeof(tempDirectory));
    if (error != ERROR_NONE) {
        return error;
    }
    error = ensureDirectory(tempDirectory);
    if (error != ERROR_NONE) {
        return error;
    }
    const auto separator = file.find_last_of('/');
    const std::string fileName = (separator == std::string::npos) ? file : file.substr(separator + 1);
    const std::string tempPath = std::string(tempDirectory) + "/" + fileName;

    error = download(app_hub_catalog_get_download_url(file).c_str(), tempPath.c_str());
    if (error == ERROR_NONE) {
        error = app_install(tempPath.c_str());
        if (error != ERROR_NONE) {
            LOG_E(TAG, "Install of %s failed: %s", id, error_to_string(error));
        }
    }
    if (path_exists(tempPath.c_str()) && file_remove(tempPath.c_str()) != ERROR_NONE) {
        LOG_W(TAG, "Failed to remove %s", tempPath.c_str());
    }
    return error;
}

error_t executeUpdateAll(const Operation& operation) {
    const auto installed = getInstalledPackages();
    std::vector<std::string> ids;
    lock();
    for (const auto& app : collectApps(installed)) {
        if (app.update_available) {
            ids.emplace_back(app.id);
        }
    }
    unlock();

    error_t lastError = ERROR_NONE;
    for (const auto& id : ids) {
        lock();
        const bool stop = stopping;
        if (!stop) {
            emitEvent(APP_HUB_EVENT_INSTALLING, operation, id.c_str(), ERROR_NONE);
        }
        unlock();
        if (stop) {
            return ERROR_INVALID_STATE;
        }

        const error_t error = executeInstall(id.c_str());
        if (error != ERROR_NONE) {
            lastError = error;
        }

        lock();
        emitEvent(APP_HUB_EVENT_INSTALLED, operation, id.c_str(), error);
        unlock();
    }
    return lastError;
}

error_t execute(const Operation& operation) {
    switch (operation.type) {
        case APP_HUB_OPERATION_REFRESH:
            return executeRefresh();
        case APP_HUB_OPERATION_INSTALL:
            return executeInstall(operation.app_id);
        case APP_HUB_OPERATION_REMOVE:
            return app_uninstall(operation.app_id);
        case APP_HUB_OPERATION_UPDATE_ALL:
            return executeUpdateAll(operation);
    }
    return ERROR_INVALID_ARGUMENT;
}

int32_t workerMain(void* /*context*/) {
    while (true) {
        lock();
        if (stopping) {
            workerRunning = false;
            unlock();
            return 0;
        }
        if (queue.empty()) {
            unlock();
            uint32_t flags = 0;
            const error_t waited = task_event_group_wait(&workerEvents, queueBit | stopBit, false, &flags, WORKER_IDLE_TIMEOUT);
            if (waited == ERROR_TIMEOUT) {
                // An operation can be queued between the wait and taking the lock
                lock();
                const bool idle = queue.empty();
                if (idle) {
                    workerRunning = false;
                }
                unlock();
                if (idle) {
                    return 0;
                }
            }
            continue;
        }

        runningOperation = queue.front();
        queue.pop_front();
        operationRunning = true;
        const Operation operation = runningOperation;
        emitEvent(getStartedEventType(operation.type), operation, operation.app_id, ERROR_NONE);
        unlock();

        const error_t result = execute(operation);

        lock();
        operationRunning = false;
        emitEvent(getFinishedEventType(operation.type), operation, operation.app_id, result);
        unlock();
    }
}

/** Requires the global lock */
error_t ensureWorkerRunning() {
    if (workerRunning) {
        return ERROR_NONE;
    }
    if (worker != nullptr) {
        // The previous worker cleared workerRunning and exits without taking the lock
        thread_join(worker, portMAX_DELAY, pdMS_TO_TICKS(10));
        thread_free(worker);
    }
    worker = thread_alloc_full("apphub", WORKER_STACK_SIZE, workerMain, nullptr, -1);
    if (worker == nullptr) {
        return ERROR_OUT_OF_MEMORY;
    }
    if (thread_start(worker) != ERROR_NONE) {
        thread_free(worker);
        worker = nullptr;
        return ERROR_RESOURCE;
    }
    workerRunning = true;
    return ERROR_NONE;
}

error_t enqueue(AppHubOperationType type, const char* id, uint32_t* operationId) {
    if (id != nullptr && (id[0] == '\0' || strlen(id) >= sizeof(PackageId))) {
        return ERROR_INVALID_ARGUMENT;
    }

    lock();
    if (!serviceRunning) {
        unlock();
        return ERROR_INVALID_STATE;
    }

    const Operation* pending = findPendingOperationOfKind(type == APP_HUB_OPERATION_REFRESH);
    if (pending != nullptr) {
        const bool same = pending->type == type && (id == nullptr || strcmp(pending->app_id, id) == 0);
        if (same && operationId != nullptr) {
            *operationId = pending->id;
        }
        unlock();
        return same ? ERROR_NONE : ERROR_RESOURCE_BUSY;
    }

    Operation operation {};
    operation.id = nextOperationId++;
    operation.type = type;
    if (id != nullptr) {
        strcpy(operation.app_id, id);
    }

    const error_t error = ensureWorkerRunning();
    if (error == ERROR_NONE) {
        queue.push_back(operation);
        task_event_group_signal(&workerEvents, queueBit);
        if (operationId != nullptr) {
            *operationId = operation.id;
        }
    }
    unlock();
    return error;
}

// endregion

// region Service

void* createService(const ServiceManifest* /*manifest*/) {
    return nullptr;
}

void destroyService(const ServiceManifest* /*manifest*/, void* /*data*/) {}

error_t onStart(ServiceInstance* /*instance*/, void* /*data*/) {
    task_event_group_construct(&workerEvents);
    if (task_event_group_claim_bit(&workerEvents, &queueBit) != ERROR_NONE ||
        task_event_group_claim_bit(&workerEvents, &stopBit) != ERROR_NONE) {
        task_event_group_destruct(&workerEvents);
        return ERROR_RESOURCE;
    }
    lock();
    serviceRunning = true;
    stopping = false;
    unlock();
    return ERROR_NONE;
}

void onStop(ServiceInstance* /*instance*/, void* /*data*/) {
    lock();
    serviceRunning = false;
    stopping = true;
    queue.clear();
    Thread* stoppingWorker = worker;
    worker = nullptr;
    unlock();

    if (stoppingWorker != nullptr) {
        task_event_group_signal(&workerEvents, stopBit);
        thread_join(stoppingWorker, portMAX_DELAY, pdMS_TO_TICKS(10));
        thread_free(stoppingWorker);
    }

    lock();
    workerRunning = false;
    unlock();
    task_event_group_destruct(&workerEvents);
}

// endregion

} // namespace

const ServiceManifest app_hub_service_manifest = {
    .id = APP_HUB_SERVICE_ID,
    .create_service = createService,
    .destroy_service = destroyService,
    .on_start = onStart,
    .on_stop = onStop,
};

bool app_hub_is_worker_running() {
    lock();
    const bool running = workerRunning;
    unlock();
    return running;
}

extern "C" {

// region Apps

error_t app_hub_search(const char* query, AppHubAppVisitorFn visitor, void* context) {
    const auto installed = getInstalledPackages();
    lock();
    AppList apps = collectApps(installed);
    std::ranges::sort(apps, lessIgnoringCase);
    for (const auto& app : apps) {
        if (query != nullptr && !containsIgnoringCase(app.name, query) && !containsIgnoringCase(app.id, query)) {
            continue;
        }
        if (!visitor(&app, context)) {
            break;
        }
    }
    unlock();
    return ERROR_NONE;
}

error_t app_hub_get_app(const char* id, AppHubApp* app) {
    const auto installed = getInstalledPackages();
    lock();
    const AppList apps = collectApps(installed);
    unlock();
    const auto found = std::ranges::find_if(apps, [id](const AppHubApp& candidate) {
        return strcmp(candidate.id, id) == 0;
    });
    if (found == apps.end()) {
        return ERROR_NOT_FOUND;
    }
    *app = *found;
    return ERROR_NONE;
}

error_t app_hub_get_description(const char* id, char* description, size_t description_size) {
    lock();
    const AppHubCatalogEntry* entry = findCatalogEntry(id);
    error_t error = ERROR_NONE;
    if (entry == nullptr) {
        error = ERROR_NOT_FOUND;
    } else if (entry->description.size() >= description_size) {
        error = ERROR_BUFFER_OVERFLOW;
    } else {
        strcpy(description, entry->description.c_str());
    }
    unlock();
    return error;
}

// endregion

// region Operations

bool app_hub_is_busy() {
    lock();
    const bool busy = operationRunning || !queue.empty();
    unlock();
    return busy;
}

error_t app_hub_refresh(uint32_t* operation_id) {
    return enqueue(APP_HUB_OPERATION_REFRESH, nullptr, operation_id);
}

error_t app_hub_install(const char* id, uint32_t* operation_id) {
    return enqueue(APP_HUB_OPERATION_INSTALL, id, operation_id);
}

error_t app_hub_update_all(uint32_t* operation_id) {
    return enqueue(APP_HUB_OPERATION_UPDATE_ALL, nullptr, operation_id);
}

error_t app_hub_remove(const char* id, uint32_t* operation_id) {
    return enqueue(APP_HUB_OPERATION_REMOVE, id, operation_id);
}

// endregion

// region Events

error_t app_hub_event_subscribe(AppHubEventSubscription* sub, TaskEventGroup* event_group) {
    uint32_t bit;
    const error_t claimed = task_event_group_claim_bit(event_group, &bit);
    if (claimed != ERROR_NONE) {
        return claimed;
    }

    lock();
    for (auto* existing = subscriptions; existing != nullptr; existing = existing->internal.next) {
        if (existing == sub) {
            unlock();
            task_event_group_release_bit(event_group, bit);
            return ERROR_INVALID_STATE;
        }
    }
    sub->bit = bit;
    sub->internal.event_group = event_group;
    sub->internal.head = 0;
    sub->internal.count = 0;
    sub->internal.next = subscriptions;
    subscriptions = sub;
    unlock();
    return ERROR_NONE;
}

error_t app_hub_event_unsubscribe(AppHubEventSubscription* sub) {
    error_t result = ERROR_NOT_FOUND;
    lock();
    for (auto** link = &subscriptions; *link != nullptr; link = &(*link)->internal.next) {
        if (*link == sub) {
            *link = sub->internal.next;
            result = ERROR_NONE;
            break;
        }
    }
    unlock();
    if (result == ERROR_NONE) {
        task_event_group_release_bit(sub->internal.event_group, sub->bit);
    }
    return result;
}

error_t app_hub_event_await(AppHubEventSubscription* sub, TickType_t timeout) {
    const TickType_t start = get_ticks();
    while (true) {
        lock();
        const bool hasEvent = sub->internal.count > 0;
        TaskEventGroup* eventGroup = sub->internal.event_group;
        unlock();
        if (hasEvent) {
            return ERROR_NONE;
        }
        const TickType_t elapsed = get_ticks() - start;
        if (elapsed >= timeout) {
            return ERROR_TIMEOUT;
        }
        // The bit can be left over from an event that was already polled, so the queue is checked again
        task_event_group_wait(eventGroup, sub->bit, false, nullptr, timeout - elapsed);
    }
}

error_t app_hub_event_poll(AppHubEventSubscription* sub, AppHubEvent* event) {
    lock();
    const bool hasEvent = sub->internal.count > 0;
    if (hasEvent) {
        *event = sub->internal.queue[sub->internal.head];
        sub->internal.head = (sub->internal.head + 1) % APP_HUB_EVENT_QUEUE_CAPACITY;
        sub->internal.count--;
    }
    unlock();
    return hasEvent ? ERROR_NONE : ERROR_TIMEOUT;
}

// endregion

}
