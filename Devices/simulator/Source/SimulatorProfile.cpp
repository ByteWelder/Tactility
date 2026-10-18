#include "SimulatorProfile.h"

#include <tactility/log.h>

#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace simulator {

constexpr auto* TAG = "SimulatorProfile";
constexpr auto* PROFILE_DIRECTORY = "data/tactility";
constexpr auto* PROFILE_PATH = "data/tactility/simulator_profile.txt";
constexpr auto* DISPLAY_SETTINGS_PATH = "data/tactility/user/app/tactility.display/display.properties";
constexpr auto* ORIENTATION_KEY = "orientation=";

static const Profile PROFILES[PROFILE_COUNT] = {
    { "Default", 640, 320, false, false, false, true, true },
    { "CL-32", 384, 168, false, true, true, true, false },
    { "LilyGO T-Deck", 320, 240, false, false, false, true, true },
    { "LilyGO T-Lora Pager", 480, 222, false, false, false, true, false },
    { "M5Stack Cardputer", 240, 135, false, true, false, true, false },
    // Half the res on each axis to compensate for DPI differences
    { "M5Stack Tab5", 640, 360, true, false, false, true, true },
};

static size_t activeProfileIndex = 0;
static char** processArguments = nullptr;
/** Absolute, because argv[0] can be a bare name that was found through PATH. Empty when it can't be resolved. */
static std::string executablePath;

const Profile& getProfile(size_t index) {
    return PROFILES[index < PROFILE_COUNT ? index : 0];
}

size_t loadSelectedProfileIndex() {
    FILE* file = fopen(PROFILE_PATH, "r");
    if (file == nullptr) {
        return 0;
    }
    char name[64] = {};
    const bool read = fgets(name, sizeof(name), file) != nullptr;
    fclose(file);
    if (!read) {
        return 0;
    }
    name[strcspn(name, "\r\n")] = '\0';
    for (size_t i = 0; i < PROFILE_COUNT; i++) {
        if (strcmp(PROFILES[i].name, name) == 0) {
            return i;
        }
    }
    return 0;
}

// A saved orientation overrides the display's default one, which the profile defines
static void clearSavedOrientation() {
    FILE* file = fopen(DISPLAY_SETTINGS_PATH, "r");
    if (file == nullptr) {
        return;
    }
    std::string kept;
    char line[256];
    while (fgets(line, sizeof(line), file) != nullptr) {
        if (strncmp(line, ORIENTATION_KEY, strlen(ORIENTATION_KEY)) != 0) {
            kept += line;
        }
    }
    fclose(file);

    file = fopen(DISPLAY_SETTINGS_PATH, "w");
    if (file == nullptr) {
        LOG_E(TAG, "Failed to write %s", DISPLAY_SETTINGS_PATH);
        return;
    }
    fputs(kept.c_str(), file);
    fclose(file);
}

bool saveSelectedProfileIndex(size_t index) {
    const bool changed = index != loadSelectedProfileIndex();
    mkdir(PROFILE_DIRECTORY, 0755);
    FILE* file = fopen(PROFILE_PATH, "w");
    if (file == nullptr) {
        LOG_E(TAG, "Failed to write %s", PROFILE_PATH);
        return false;
    }
    const bool written = fprintf(file, "%s\n", getProfile(index).name) > 0;
    if (fclose(file) != 0 || !written) {
        return false;
    }
    if (changed) {
        clearSavedOrientation();
    }
    return true;
}

void initActiveProfile() {
    activeProfileIndex = loadSelectedProfileIndex();
    LOG_I(TAG, "Profile: %s", getProfile(activeProfileIndex).name);
}

size_t getActiveProfileIndex() {
    return activeProfileIndex;
}

static std::string resolveExecutablePath(const char* argv0) {
    char path[PATH_MAX];
#ifdef __APPLE__
    uint32_t size = sizeof(path);
    char resolved[PATH_MAX];
    if (_NSGetExecutablePath(path, &size) == 0 && realpath(path, resolved) != nullptr) {
        return resolved;
    }
#else
    const ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (length > 0) {
        path[length] = '\0';
        return path;
    }
#endif
    // Only a path with a slash can be resolved without searching PATH
    if (strchr(argv0, '/') != nullptr && realpath(argv0, path) != nullptr) {
        return path;
    }
    return "";
}

void setProcessArguments(char** argv) {
    processArguments = argv;
    executablePath = resolveExecutablePath(argv[0]);
}

void reboot() {
    if (processArguments == nullptr) {
        LOG_E(TAG, "Cannot reboot without process arguments");
        return;
    }
    // The FreeRTOS tick timer would outlive exec() and kill the new image with SIGALRM
    itimerval previousTimer = {};
    itimerval stopTimer = {};
    setitimer(ITIMER_REAL, &stopTimer, &previousTimer);
    if (!executablePath.empty()) {
        execv(executablePath.c_str(), processArguments);
    } else {
        execvp(processArguments[0], processArguments);
    }
    LOG_E(TAG, "Failed to reboot: %s", strerror(errno));
    // This process keeps running, so it needs its ticks again
    setitimer(ITIMER_REAL, &previousTimer, nullptr);
}

}
