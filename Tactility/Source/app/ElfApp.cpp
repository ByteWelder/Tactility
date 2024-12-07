#ifdef ESP_PLATFORM

#include "file/File.h"
#include "ElfApp.h"
#include "TactilityCore.h"
#include "esp_elf.h"
#include "ElfSymbols.h"

namespace tt::app {

#define TAG "elf_app"

bool startElfApp(const char* filePath) {
    TT_LOG_I(TAG, "Starting ELF %s", filePath);

    // TODO: Move
    initElfSymbols();

    size_t size = 0;
    auto elf_file_data = file::readBinary(filePath, size);
    if (elf_file_data == nullptr) {
        return false;
    }

    esp_elf_t elf;
    if (esp_elf_init(&elf) < 0) {
        TT_LOG_E(TAG, "Failed to initialize");
        return false;
    }

    if (esp_elf_relocate(&elf, elf_file_data.get()) < 0) {
        TT_LOG_E(TAG, "Failed to load executable");
        return false;
    }

    int argc = 0;
    char* argv[] = {};

    if (esp_elf_request(&elf, 0, argc, argv) < 0) {
        TT_LOG_W(TAG, "Executable returned error code");
        return false;
    }

    esp_elf_deinit(&elf);
    return true;
}

} // namespace

#endif // ESP_PLATFORM