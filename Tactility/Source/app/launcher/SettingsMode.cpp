#include <Tactility/app/AppIcon.h>
#include <Tactility/app/launcher/LauncherMode.h>

#include <app/manager.h>
#include <app/manifest.h>

#include <lvgl/icons/shared.h>

#include <algorithm>
#include <cstring>

namespace tt::app::launcher {

namespace {

void collectManifest(const ::AppManifest* manifest, void* context) {
    auto* manifests = static_cast<std::vector<::AppManifest>*>(context);
    manifests->push_back(*manifest);
}

std::vector<TileGridItem> collectItems() {
    std::vector<::AppManifest> collected;
    app_manager_for_each_manifest(collectManifest, &collected);
    std::ranges::sort(collected, [](const ::AppManifest& a, const ::AppManifest& b) {
        return strcmp(a.name, b.name) < 0;
    });

    std::vector<TileGridItem> items;
    for (const auto& manifest : collected) {
        if (manifest.category == APP_CATEGORY_SETTINGS && (manifest.flags & APP_MANIFEST_FLAG_HIDDEN) == 0) {
            items.push_back({ manifest.id, manifest.name, tt::app::getAppIcon(manifest), false });
        }
    }
    return items;
}

} // namespace

const LauncherMode SETTINGS_MODE = {
    .buttonIcon = LVGL_ICON_SHARED_SETTINGS,
    .iconColor = TileGrid::IconColor::Secondary,
    .collect = collectItems,
    .onLongPressed = nullptr,
    .onKey = nullptr
};

}
