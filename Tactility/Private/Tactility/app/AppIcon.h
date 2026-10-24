#pragma once

#include <app/manifest.h>

namespace tt::app {

/**
 * The shared icon font character for an app, as the launcher and the recents switcher show it.
 * @param[in] manifest the app to look up
 * @return the icon, or a generic one for apps without a specific icon
 */
const char* getAppIcon(const ::AppManifest& manifest);

}
