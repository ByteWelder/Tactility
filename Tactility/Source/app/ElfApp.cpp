#ifdef ESP_PLATFORM

#include "ElfApp.h"
#include "Log.h"
#include "StringUtils.h"
#include "TactilityCore.h"
#include "esp_elf.h"
#include "file/File.h"
#include "service/loader/Loader.h"

#include <string>

namespace tt::app {

#define TAG "elf_app"

struct ElfManifest {
    /** The user-readable name of the app. Used in UI. */
    std::string name;
    /** Optional icon. */
    std::string icon;
    CreateData _Nullable createData;
    DestroyData _Nullable destroyData;
    OnStart _Nullable onStart;
    OnStop _Nullable onStop;
    OnShow _Nullable onShow;
    OnHide _Nullable onHide;
    OnResult _Nullable onResult;
};

static size_t elfManifestSetCount = 0;
static ElfManifest elfManifest;

class ElfApp : public App {

private:

    const std::string filePath;
    std::unique_ptr<uint8_t[]> elfFileData;
    esp_elf_t elf;
    bool shouldCleanupElf = false; // Whether we have to clean up the above "elf" object
    std::unique_ptr<ElfManifest> manifest;
    void* data = nullptr;

    bool startElf() {
        TT_LOG_I(TAG, "Starting ELF %s", filePath.c_str());
        assert(elfFileData == nullptr);

        size_t size = 0;
        elfFileData = file::readBinary(filePath, size);
        if (elfFileData == nullptr) {
            return false;
        }

        if (esp_elf_init(&elf) < 0) {
            TT_LOG_E(TAG, "Failed to initialize");
            shouldCleanupElf = true;
            return false;
        }

        if (esp_elf_relocate(&elf, elfFileData.get()) < 0) {
            TT_LOG_E(TAG, "Failed to load executable");
            return false;
        }

        int argc = 0;
        char* argv[] = {};

        if (esp_elf_request(&elf, 0, argc, argv) < 0) {
            TT_LOG_W(TAG, "Executable returned error code");
            return false;
        }

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

    explicit ElfApp(const std::string& filePath) : filePath(filePath) {}

    void onStart(AppContext& appContext) override {
        auto initial_count = elfManifestSetCount;
        if (startElf()) {
            if (elfManifestSetCount > initial_count) {
                manifest = std::make_unique<ElfManifest>(elfManifest);

                if (manifest->createData != nullptr) {
                    data = manifest->createData();
                }

                if (manifest->onStart != nullptr) {
                    manifest->onStart(appContext, data);
                }
            }
        } else {
            service::loader::stopApp();
        }
    }

    void onStop(AppContext& appContext) override {
        TT_LOG_I(TAG, "Cleaning up app");
        if (manifest != nullptr) {
            if (manifest->onStop != nullptr) {
                manifest->onStop(appContext, data);
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
            manifest->onShow(appContext, data, parent);
        }
    }

    void onHide(AppContext& appContext) override {
        if (manifest != nullptr && manifest->onHide != nullptr) {
            manifest->onHide(appContext, data);
        }
    }

    void onResult(AppContext& appContext, Result result, std::unique_ptr<Bundle> resultBundle) override {
        if (manifest != nullptr && manifest->onResult != nullptr) {
            manifest->onResult(appContext, data, result, std::move(resultBundle));
        }
    }
};

void setElfAppManifest(
    const char* name,
    const char* _Nullable icon,
    CreateData _Nullable createData,
    DestroyData _Nullable destroyData,
    OnStart _Nullable onStart,
    OnStop _Nullable onStop,
    OnShow _Nullable onShow,
    OnHide _Nullable onHide,
    OnResult _Nullable onResult
) {
    elfManifest = ElfManifest {
        .name = name ? name : "",
        .icon = icon ? icon : "",
        .createData = createData,
        .destroyData = destroyData,
        .onStart = onStart,
        .onStop = onStop,
        .onShow = onShow,
        .onHide = onHide,
        .onResult = onResult
    };
    elfManifestSetCount++;
}

std::string getElfAppId(const std::string& filePath) {
    return filePath;
}

bool registerElfApp(const std::string& filePath) {
    if (findAppById(filePath) == nullptr) {
        auto manifest = AppManifest {
            .id = getElfAppId(filePath),
            .name = tt::string::removeFileExtension(tt::string::getLastPathSegment(filePath)),
            .type = Type::User,
            .location = Location::external(filePath)
        };
        addApp(manifest);
    }
    return false;
}

std::shared_ptr<App> createElfApp(const std::shared_ptr<AppManifest>& manifest) {
    TT_LOG_I(TAG, "createElfApp");
    assert(manifest != nullptr);
    assert(manifest->location.isExternal());
    return std::make_shared<ElfApp>(manifest->location.getPath());
}

} // namespace

#endif // ESP_PLATFORM
