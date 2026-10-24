#include <Tactility/app/AppIcon.h>
#include <Tactility/app/launcher/Favourites.h>
#include <Tactility/app/launcher/LauncherMode.h>

#include <app/manager.h>
#include <app/manifest.h>
#include <app/package_manifest.h>

#include <lvgl/icons/shared.h>
#include <tactility/file_ops.h>
#include <tactility/log.h>

#include <algorithm>
#include <cstring>
#include <new>
#include <string>

namespace tt::app::launcher {

namespace {

constexpr auto* TAG = "Launcher";

const Favourites favourites;

struct SaveFavouritesJob {
    std::vector<std::string> ids;
};

// File operation: runs on the file operations task, which frees the job
void saveFavourites(void* context) {
    auto* job = static_cast<SaveFavouritesJob*>(context);
    if (!favourites.save(job->ids)) {
        LOG_E(TAG, "Failed to save favourites");
    }
    delete job;
}

bool isShown(const ::AppManifest& manifest) {
    if (manifest.category != APP_CATEGORY_USER && manifest.category != APP_CATEGORY_SYSTEM) {
        return false;
    }
    if ((manifest.flags & APP_MANIFEST_FLAG_HEADLESS) != 0) {
        return false;
    }
    if ((manifest.flags & APP_MANIFEST_FLAG_HIDDEN) != 0) {
        return false;
    }

    return true;
}

void collectManifest(const ::AppManifest* manifest, void* context) {
    auto* manifests = static_cast<std::vector<::AppManifest>*>(context);
    manifests->push_back(*manifest);
}

// Apps from installed packages this device can't run (wrong device, too little RAM)
void collectIncompatibleAppIds(const AppPackage* package, void* context) {
    if (app_package_manifest_is_compatible(&package->package)) {
        return;
    }
    auto* ids = static_cast<std::vector<std::string>*>(context);
    for (size_t i = 0; i < package->app_id_count; i++) {
        ids->emplace_back(package->app_ids[i]);
    }
}

std::vector<TileGridItem> collectItems() {
    const std::vector<std::string> favouriteIds = favourites.ids();

    std::vector<::AppManifest> collected;
    app_manager_for_each_manifest(collectManifest, &collected);
    std::vector<std::string> incompatibleIds;
    app_manager_for_each_package(collectIncompatibleAppIds, &incompatibleIds);
    ::std::erase_if(collected, [&](const ::AppManifest& manifest) {
        return !isShown(manifest) || ::std::ranges::find(incompatibleIds, std::string(manifest.id)) != incompatibleIds.end();
    });

    std::vector<TileGridItem> items;
    items.reserve(collected.size());
    for (const auto& manifest : collected) {
        items.push_back({ manifest.id, manifest.name, tt::app::getAppIcon(manifest), Favourites::contains(favouriteIds, manifest.id) });
    }
    ::std::ranges::sort(items, [](const TileGridItem& a, const TileGridItem& b) {
        if (a.highlighted != b.highlighted) {
            return a.highlighted;
        }
        return a.name < b.name;
    });
    return items;
}

void toggleFavourite(TileGrid& grid, const char* appId, bool keepSelection) {
    auto* job = new (::std::nothrow) SaveFavouritesJob();
    if (job == nullptr) {
        LOG_E(TAG, "Failed to queue saving favourites");
        return;
    }
    if (!favourites.toggle(appId, job->ids)) {
        // The favourites are still being loaded, so toggling would overwrite the file with an incomplete list
        LOG_W(TAG, "Favourites aren't loaded yet");
        delete job;
        return;
    }
    if (file_ops_queue(saveFavourites, job, nullptr) != ERROR_NONE) {
        LOG_E(TAG, "Failed to queue saving favourites");
        delete job;
    }
    grid.requestRepopulate(keepSelection);
}

void onLongPressed(TileGrid& grid, const TileGridItem& item) {
    toggleFavourite(grid, item.id.c_str(), false);
}

void onKey(TileGrid& grid, const TileGridItem& item, uint32_t key) {
    if (key == 'f' || key == 'F') {
        toggleFavourite(grid, item.id.c_str(), true);
    }
}

} // namespace

void loadFavourites(void*) {
    favourites.load();
}

const LauncherMode APPS_MODE = {
    .buttonIcon = LVGL_ICON_SHARED_APPS,
    .iconColor = TileGrid::IconColor::Primary,
    .collect = collectItems,
    .onLongPressed = onLongPressed,
    .onKey = onKey
};

}
