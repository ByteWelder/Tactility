#ifdef ESP_PLATFORM

#include "file/File.h"
#include "ElfApp.h"
#include "TactilityCore.h"
#include "esp_elf.h"
#include "service/loader/Loader.h"

#include <string>

namespace tt::app {

#define TAG "elf_app"

struct ElfManifest {
    /** The user-readable name of the app. Used in UI. */
    std::string name;
    /** Optional icon. */
    std::string icon;
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

    std::string filePath;
    std::unique_ptr<uint8_t[]> elfFileData;
    esp_elf_t elf;
    std::unique_ptr<ElfManifest> manifest;

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
            return false;
        }

        if (esp_elf_relocate(&elf, elfFileData.get()) < 0) {
            TT_LOG_E(TAG, "Failed to load executable");
            return false;
        }

        int argc = 0;
        char* argv[] = {};

        size_t manifest_set_count = elfManifestSetCount;
        if (esp_elf_request(&elf, 0, argc, argv) < 0) {
            TT_LOG_W(TAG, "Executable returned error code");
            return false;
        }

        return true;
    }

    void stopElf() {
        TT_LOG_I(TAG, "Cleaning up ELF");
        if (elfFileData != nullptr) {
            esp_elf_deinit(&elf);
            elfFileData = nullptr;
        }
    }

public:

    ElfApp(std::string filePath) : filePath(std::move(filePath)) {}

    void onStart(AppContext& appContext) override {
        auto initial_count = elfManifestSetCount;
        if (startElf()) {
            if (elfManifestSetCount > initial_count) {
                manifest = std::make_unique<ElfManifest>(elfManifest);
                manifest->onStart(appContext);
//                service::loader::startApp(ELF_WRAPPER_APP_ID);
            }
        }
    }

    void onStop(AppContext& appContext) override {
        TT_LOG_I(TAG, "Cleaning up app");
        if (manifest != nullptr) {
            manifest->onStop(appContext);
            this->manifest = nullptr;
        }
        stopElf();
    }

    void onShow(AppContext& appContext, lv_obj_t* parent) override {
        if (manifest != nullptr) {
            manifest->onShow(appContext, parent);
        }
    }

    void onHide(AppContext& appContext) override {
        if (manifest != nullptr) {
            manifest->onHide(appContext);
        }
    }

    void onResult(AppContext& appContext, Result result, const Bundle& resultBundle) override {
        if (manifest != nullptr) {
            manifest->onResult(appContext, result, resultBundle);
        }
    }
};

void setElfAppManifest(
    const char* name,
    const char* _Nullable icon,
    OnStart _Nullable onStart,
    OnStop _Nullable onStop,
    OnShow _Nullable onShow,
    OnHide _Nullable onHide,
    OnResult _Nullable onResult
) {
    elfManifest = ElfManifest {
        .name = name ? name : "",
        .icon = icon ? icon : "",
        .onStart = onStart,
        .onStop = onStop,
        .onShow = onShow,
        .onHide = onHide,
        .onResult = onResult
    };
    elfManifestSetCount++;
}

bool startElfApp(const std::string& filePath) {
    // TODO: Register app in registry, then start it
    return false;
}

} // namespace

#endif // ESP_PLATFORM
