#include <tactility/device.h>
#include <tactility/drivers/display.h>
#include <tactility/drivers/spi_controller.h>
#include <tactility/filesystem/file_lock.h>
#include <tactility/filesystem/file_system.h>
#include <tactility/lvgl_module.h>


struct Device;
namespace tt {

static const FileMutex lvgl_mutex = {
    .lock = lvgl_lock,
    .try_lock = lvgl_try_lock,
    .unlock = lvgl_unlock,
};

/**
 * Finds file systems with a device (e.g. sd card) that is owned by a SPI controller.
 * If the SPI controller has a display on the bus, we create an LVGL lock for the file system path.
 */
void initFileLvglLock() {
    file_system_for_each(nullptr, [](FileSystem* fs, void* context) {
        char mount_path[64];
        if (file_system_get_path(fs, mount_path, sizeof(mount_path)) != ERROR_NONE) {
            return true;
        }

        // We only care about file system with a Device (owner)
        auto* owner = file_system_get_owner(fs);
        if (owner == nullptr) {
            return true;
        }

        // Ignore devices without a parent (root)
        auto* parent = device_get_parent(owner);
        if (parent == nullptr) {
            return true;
        }

        // If the FileSystem is on a SPI bus and there's more than 1 device, we assume the other one is the display.
        auto* type = device_get_type(parent);
        if (type != &SPI_CONTROLLER_TYPE || device_get_child_count(parent) <= 1) {
            return true;
        }

        struct Context {
            const char* mountPath;
        };
        Context ctx = { .mountPath = mount_path };

        device_for_each_child(parent, &ctx, [](Device* child, void* context) {
            Context* ctx = static_cast<Context*>(context);
            if (device_get_type(child) == &DISPLAY_TYPE) {
                file_register_mutex(
                    ctx->mountPath,
                    &lvgl_mutex
                );
                return false;
            }
            return true;
        });

        return true;
    });

}

}
