#ifdef ESP_PLATFORM

#include "Tactility/app/ElfApp.h"
#include "Tactility/file/File.h"
#include "Tactility/file/FileLock.h"
#include "Tactility/service/loader/Loader.h"

#include <Tactility/Log.h>
#include <Tactility/StringUtils.h>

#include "esp_elf.h"

#include <string>
#include <utility>
#include <Tactility/app/alertdialog/AlertDialog.h>

namespace tt::app {

constexpr auto* TAG = "ElfApp";

struct ElfManifest {
    /** The user-readable name of the app. Used in UI. */
    std::string name;
    /** Optional icon. */
    std::string icon;
    CreateData _Nullable createData = nullptr;
    DestroyData _Nullable destroyData = nullptr;
    OnCreate _Nullable onCreate = nullptr;
    OnDestroy _Nullable onDestroy = nullptr;
    OnShow _Nullable onShow = nullptr;
    OnHide _Nullable onHide = nullptr;
    OnResult _Nullable onResult = nullptr;
};

static size_t elfManifestSetCount = 0;
static ElfManifest elfManifest;
static std::shared_ptr<Lock> elfManifestLock = std::make_shared<Mutex>();

class ElfApp : public App {

    const std::string appPath;
    std::unique_ptr<uint8_t[]> elfFileData;
    esp_elf_t elf {
        .psegment = nullptr,
        .svaddr = 0,
        .ptext = nullptr,
        .pdata = nullptr,
        .sec = { },
        .entry = nullptr
    };
    bool shouldCleanupElf = false; // Whether we have to clean up the above "elf" object
    std::unique_ptr<ElfManifest> manifest;
    void* data = nullptr;
    std::string lastError = "";

    bool startElf() {
        const std::string elf_path = std::format("{}/elf/{}.elf", appPath, CONFIG_IDF_TARGET);
        TT_LOG_I(TAG, "Starting ELF %s", elf_path.c_str());
        assert(elfFileData == nullptr);

        size_t size = 0;
        file::withLock<void>(elf_path, [this, &elf_path, &size]{
            elfFileData = file::readBinary(elf_path, size);
        });

        if (elfFileData == nullptr) {
            return false;
        }

        if (esp_elf_init(&elf) != ESP_OK) {
            lastError = "Failed to initialize";
            TT_LOG_E(TAG, "%s", lastError.c_str());
            elfFileData  = nullptr;
            return false;
        }

        auto relocate_result = esp_elf_relocate(&elf, elfFileData.get());
        if (relocate_result != 0) {
            // Note: the result code mapes to values from cstdlib's errno.h
            lastError = std::format("Failed to load executable (error code {})", -relocate_result);
            TT_LOG_E(TAG, "%s", lastError.c_str());
            elfFileData  = nullptr;
            return false;
        }

        int argc = 0;
        char* argv[] = {};

        if (esp_elf_request(&elf, 0, argc, argv) != ESP_OK) {
            lastError = "Executable returned error code";
            TT_LOG_E(TAG, "%s", lastError.c_str());
            esp_elf_deinit(&elf);
            elfFileData  = nullptr;
            return false;
        }

        shouldCleanupElf = true;
        return true;
    }

    void stopElf() {
        TT_LOG_I(TAG, "Cleaning up ELF");

        if (shouldCleanupElf) {
            esp_elf_deinit(&elf);
        }

        if (elfFileData != nullptr) {
            elfFileData = nullptr;
        }
    }

public:

    explicit ElfApp(std::string appPath) : appPath(std::move(appPath)) {}

    void onCreate(AppContext& appContext) override {
        // Because we use global variables, we have to ensure that we are not starting 2 apps in parallel
        // We use a ScopedLock so we don't have to safeguard all branches
        auto lock = elfManifestLock->asScopedLock();
        lock.lock();

        elfManifestSetCount = 0;
        if (!startElf()) {
            service::loader::stopApp();
            auto message = lastError.empty() ? "Application failed to start." : std::format("Application failed to start: {}", lastError);
            alertdialog::start("Error", message);
            return;
        }

        if (elfManifestSetCount == 0) {
            service::loader::stopApp();
            alertdialog::start("Error", "Application failed to start: application failed to register itself");
            return;
        }

        manifest = std::make_unique<ElfManifest>(elfManifest);
        lock.unlock();

        if (manifest->createData != nullptr) {
            data = manifest->createData();
        }

        if (manifest->onCreate != nullptr) {
            manifest->onCreate(&appContext, data);
        }
    }

    void onDestroy(AppContext& appContext) override {
        TT_LOG_I(TAG, "Cleaning up app");
        if (manifest != nullptr) {
            if (manifest->onDestroy != nullptr) {
                manifest->onDestroy(&appContext, data);
            }

            if (manifest->destroyData != nullptr && data != nullptr) {
                manifest->destroyData(data);
            }

            this->manifest = nullptr;
        }
        stopElf();
    }

    void onShow(AppContext& appContext, lv_obj_t* parent) override {
        if (manifest != nullptr && manifest->onShow != nullptr) {
            manifest->onShow(&appContext, data, parent);
        }
    }

    void onHide(AppContext& appContext) override {
        if (manifest != nullptr && manifest->onHide != nullptr) {
            manifest->onHide(&appContext, data);
        }
    }

    void onResult(AppContext& appContext, LaunchId launchId, Result result, std::unique_ptr<Bundle> resultBundle) override {
        if (manifest != nullptr && manifest->onResult != nullptr) {
            manifest->onResult(&appContext, data, launchId, result, resultBundle.get());
        }
    }
};

void setElfAppManifest(
    const char* name,
    const char* _Nullable icon,
    CreateData _Nullable createData,
    DestroyData _Nullable destroyData,
    OnCreate _Nullable onCreate,
    OnDestroy _Nullable onDestroy,
    OnShow _Nullable onShow,
    OnHide _Nullable onHide,
    OnResult _Nullable onResult
) {
    elfManifest = ElfManifest {
        .name = name ? name : "",
        .icon = icon ? icon : "",
        .createData = createData,
        .destroyData = destroyData,
        .onCreate = onCreate,
        .onDestroy = onDestroy,
        .onShow = onShow,
        .onHide = onHide,
        .onResult = onResult
    };
    elfManifestSetCount++;
}

std::shared_ptr<App> createElfApp(const std::shared_ptr<AppManifest>& manifest) {
    TT_LOG_I(TAG, "createElfApp");
    assert(manifest != nullptr);
    assert(manifest->location.isExternal());
    return std::make_shared<ElfApp>(manifest->location.getPath());
}

} // namespace

#endif // ESP_PLATFORM
