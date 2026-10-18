#pragma once

#include <app/manifest.h>

#include <cstddef>
#include <cstdint>

namespace simulator {

extern const ::AppManifest profileAppManifest;

struct Profile {
    const char* name;
    uint16_t width;
    uint16_t height;
    /** The default rotation: the display is taller than wide. The width and height values may be given in either order. */
    bool portrait;
    /** Compact UI density */
    bool compact;
    /** Greyscale display, which makes the monochrome theme apply */
    bool mono;
    bool keyboard;
    bool pointer;
};

constexpr size_t PROFILE_COUNT = 6;

const Profile& getProfile(size_t index);

/** Reads the selected profile and makes it the active one. Call once at startup. */
void initActiveProfile();

/** The profile that this run uses */
size_t getActiveProfileIndex();

/** The profile that the next run uses */
size_t loadSelectedProfileIndex();

bool saveSelectedProfileIndex(size_t index);

void setProcessArguments(char** argv);

/** Restarts the process. Returns only when that failed. */
void reboot();

}
