#include <tactility/device.h>
#include <tactility/drivers/display.h>
#include <tactility/drivers/spi_controller.h>
#include <tactility/filesystem/file_mutex.h>
#include <tactility/filesystem/file_system.h>
#include <lvgl/lvgl.h>

constexpr auto* TAG = "file_mutex_lvgl";

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
void initFileMutexForLvgl() {
    file_system_for_each(nullptr, [](FileSystem* fs, void* context) {
        char mount_path[64];
        if (file_system_get_path(fs, mount_path, sizeof(mount_path)) != ERROR_NONE) {
            return true;
        }
        LOG_D(TAG, "Mount path %s", mount_path);

        // We only care about file system with a Device (owner)
        auto* owner = file_system_get_owner(fs);
        if (owner == nullptr) {
            LOG_D(TAG, "Owner: none");
            return true;
        }

        LOG_D(TAG, "Owner: %s", owner->name);

        // Ignore devices without a parent (root)
        auto* parent = device_get_parent(owner);
        if (parent == nullptr) {
            LOG_D(TAG, "Owner: no parent");
            return true;
        }

        LOG_D(TAG, "Owner: parent %s", parent->name);

        // If the FileSystem is on a SPI bus and there's more than 1 device, we assume the other one is the display.
        auto* type = device_get_type(parent);
        if (type != &SPI_CONTROLLER_TYPE || device_get_child_count(parent) <= 1) {
            LOG_D(TAG, "Owner parent not SPI controller or not enough children");
            return true;
        }

        struct Context {
            const char* mountPath;
        };
        Context ctx = { .mountPath = mount_path };

        device_for_each_child(parent, &ctx, [](Device* child, void* context) -> bool {
            Context* ctx = static_cast<Context*>(context);
            if (device_get_type(child) == &DISPLAY_TYPE) {
                LOG_I(TAG, "Adding file mutex for %s as it shares a bus with a display", ctx->mountPath);
                file_mutex_register(
                    &lvgl_mutex,
                    ctx->mountPath
                );
                return false;
            } else {
                LOG_D(TAG, "child of parent, %s: not DISPLAY_TYPE", child->name);
            }
            return true;
        });

        return true;
    });
}

}
