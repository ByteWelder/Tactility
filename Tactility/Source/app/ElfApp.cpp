#ifdef ESP_PLATFORM

#include "file/File.h"
#include "ElfApp.h"
#include "TactilityCore.h"
#include "esp_elf.h"

#include "service/loader/Loader.h"

namespace tt::app {

#define TAG "elf_app"
#define ELF_WRAPPER_APP_ID "ElfWrapper"

static size_t elfManifestSetCount = 0;
std::unique_ptr<uint8_t[]> elfFileData;
esp_elf_t elf;

bool startElfApp(const std::string& filePath) {
    TT_LOG_I(TAG, "Starting ELF %s", filePath.c_str());

    assert(elfFileData == nullptr);

    size_t size = 0;
    elfFileData = file::readBinary(filePath.c_str(), size);
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

    if (elfManifestSetCount > manifest_set_count) {
        service::loader::startApp(ELF_WRAPPER_APP_ID);
    } else {
        TT_LOG_W(TAG, "App did not set manifest to run - cleaning up ELF");
        esp_elf_deinit(&elf);
        elfFileData = nullptr;
    }

    return true;
}

static void onStart(AppContext& app) {}
static void onStop(AppContext& app) {}
static void onShow(AppContext& app, lv_obj_t* parent) {}
static void onHide(AppContext& app) {}
static void onResult(AppContext& app, Result result, const Bundle& resultBundle) {}

AppManifest elfManifest = {
    .id = "",
    .name = "",
    .type = TypeHidden,
    .onStart = onStart,
    .onStop = onStop,
    .onShow = onShow,
    .onHide = onHide,
    .onResult = onResult
};

static void onStartWrapper(AppContext& app) {
    elfManifest.onStart(app);
}

static void onStopWrapper(AppContext& app) {
    elfManifest.onStop(app);
    TT_LOG_I(TAG, "Cleaning up ELF");
    esp_elf_deinit(&elf);
    elfFileData = nullptr;
}

static void onShowWrapper(AppContext& app, lv_obj_t* parent) {
    elfManifest.onShow(app, parent);
}

static void onHideWrapper(AppContext& app) {
    elfManifest.onHide(app);
}

static void onResultWrapper(AppContext& app, Result result, const Bundle& bundle) {
    elfManifest.onResult(app, result, bundle);
}

AppManifest elfWrapperManifest = {
    .id = ELF_WRAPPER_APP_ID,
    .name = "ELF Wrapper",
    .type = TypeHidden,
    .onStart = onStartWrapper,
    .onStop = onStopWrapper,
    .onShow = onShowWrapper,
    .onHide = onHideWrapper,
    .onResult = onResultWrapper
};

void setElfAppManifest(const AppManifest& manifest) {
    elfManifest.id = manifest.id;
    elfManifest.name = manifest.name;
    elfWrapperManifest.name = manifest.name;
    elfManifest.onStart = manifest.onStart;
    elfManifest.onStop = manifest.onStop;
    elfManifest.onShow = manifest.onShow;
    elfManifest.onHide = manifest.onHide;
    elfManifest.onResult = manifest.onResult;

    elfManifestSetCount++;
}

} // namespace

#endif // ESP_PLATFORM