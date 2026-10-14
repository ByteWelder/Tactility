// SPDX-License-Identifier: Apache-2.0
#include <development/trust.h>

#include <tactility/concurrent/mutex.h>
#include <tactility/filesystem/fs.h>
#include <tactility/log.h>
#include <tactility/paths.h>

#include <cstdio>
#include <cstring>

namespace {

constexpr auto* TAG = "DevelopmentTrust";
constexpr auto* TRUST_FILE = "trusted_clients.txt";
constexpr auto* TRUST_FILE_TEMPORARY = "trusted_clients.txt.tmp";

// File format: one "<fingerprint> <client name>" line per client
constexpr size_t LINE_MAX_LENGTH = DEVELOPMENT_SERVER_FINGERPRINT_LENGTH + 1 + DEVELOPMENT_SERVER_CLIENT_NAME_LENGTH;

struct GlobalMutex {
    Mutex handle {};
    GlobalMutex() { mutex_construct(&handle); }
    ~GlobalMutex() { mutex_destruct(&handle); }
};

// Sessions and the UI read and change the file concurrently
GlobalMutex fileMutex;

bool getPath(const char* directory, const char* file, char* outPath, size_t outSize) {
    const int written = snprintf(outPath, outSize, "%s/%s", directory, file);
    return written > 0 && static_cast<size_t>(written) < outSize;
}

/**
 * Splits a line into its fingerprint and name, in place.
 * @return false when the line isn't valid
 */
bool parseLine(char* line, const char** outName) {
    line[strcspn(line, "\r\n")] = '\0';
    const size_t length = strlen(line);
    if (length < DEVELOPMENT_SERVER_FINGERPRINT_LENGTH ||
        (length > DEVELOPMENT_SERVER_FINGERPRINT_LENGTH && line[DEVELOPMENT_SERVER_FINGERPRINT_LENGTH] != ' ')) {
        return false;
    }
    line[DEVELOPMENT_SERVER_FINGERPRINT_LENGTH] = '\0';
    *outName = length > DEVELOPMENT_SERVER_FINGERPRINT_LENGTH ? line + DEVELOPMENT_SERVER_FINGERPRINT_LENGTH + 1 : "";
    return true;
}

/** @return false when the file couldn't be read, which is fine when it doesn't exist */
bool forEachUnlocked(const char* directory, DevelopmentTrustedClientVisitor visitor, void* context) {
    char path[FILE_MAX_PATH_STRING_LENGTH];
    if (!getPath(directory, TRUST_FILE, path, sizeof(path))) {
        return false;
    }
    FILE* file = fopen(path, "r");
    if (file == nullptr) {
        return false;
    }
    char line[LINE_MAX_LENGTH + 2];
    while (fgets(line, sizeof(line), file) != nullptr) {
        const char* name;
        if (parseLine(line, &name) && !visitor(line, name, context)) {
            break;
        }
    }
    fclose(file);
    return true;
}

bool containsUnlocked(const char* directory, const char* fingerprint) {
    struct Search {
        const char* fingerprint;
        bool found;
    } search { fingerprint, false };
    forEachUnlocked(directory, [](const char* storedFingerprint, const char*, void* context) {
        auto* search = static_cast<Search*>(context);
        search->found = strcmp(storedFingerprint, search->fingerprint) == 0;
        return !search->found;
    }, &search);
    return search.found;
}

/** Copies @a name without characters that would break the file format */
void sanitizeName(const char* name, char* outName) {
    size_t i = 0;
    for (; name[i] != '\0' && i < DEVELOPMENT_SERVER_CLIENT_NAME_LENGTH; i++) {
        outName[i] = (name[i] < ' ' || name[i] == 0x7F) ? '?' : name[i];
    }
    outName[i] = '\0';
}

error_t addUnlocked(const char* directory, const char* fingerprint, const char* name) {
    if (containsUnlocked(directory, fingerprint)) {
        return ERROR_NONE;
    }
    char path[FILE_MAX_PATH_STRING_LENGTH];
    if (!getPath(directory, TRUST_FILE, path, sizeof(path))) {
        return ERROR_BUFFER_OVERFLOW;
    }
    const error_t directoryError = directory_make(directory, true);
    if (directoryError != ERROR_NONE && directoryError != ERROR_ALREADY_EXISTS) {
        return ERROR_RESOURCE;
    }
    FILE* file = fopen(path, "a");
    if (file == nullptr) {
        LOG_E(TAG, "Failed to open %s", path);
        return ERROR_RESOURCE;
    }
    char safeName[DEVELOPMENT_SERVER_CLIENT_NAME_LENGTH + 1];
    sanitizeName(name, safeName);
    const bool written = fprintf(file, "%s %s\n", fingerprint, safeName) > 0;
    return (fclose(file) == 0 && written) ? ERROR_NONE : ERROR_RESOURCE;
}

/** Rewrites the file without @a fingerprint, through a temporary file so a failure keeps the old one */
error_t removeUnlocked(const char* directory, const char* fingerprint) {
    char path[FILE_MAX_PATH_STRING_LENGTH];
    char temporaryPath[FILE_MAX_PATH_STRING_LENGTH];
    if (!getPath(directory, TRUST_FILE, path, sizeof(path)) || !getPath(directory, TRUST_FILE_TEMPORARY, temporaryPath, sizeof(temporaryPath))) {
        return ERROR_BUFFER_OVERFLOW;
    }
    FILE* temporary = fopen(temporaryPath, "w");
    if (temporary == nullptr) {
        return ERROR_RESOURCE;
    }
    struct Rewrite {
        const char* fingerprint;
        FILE* file;
        bool found;
        bool failed;
    } rewrite { fingerprint, temporary, false, false };
    forEachUnlocked(directory, [](const char* storedFingerprint, const char* name, void* context) {
        auto* rewrite = static_cast<Rewrite*>(context);
        if (strcmp(storedFingerprint, rewrite->fingerprint) == 0) {
            rewrite->found = true;
        } else if (fprintf(rewrite->file, "%s %s\n", storedFingerprint, name) < 0) {
            rewrite->failed = true;
        }
        return !rewrite->failed;
    }, &rewrite);
    if (fclose(temporary) != 0) {
        rewrite.failed = true;
    }
    if (rewrite.failed || !rewrite.found) {
        remove(temporaryPath);
        return rewrite.failed ? ERROR_RESOURCE : ERROR_NOT_FOUND;
    }
    if (rename(temporaryPath, path) != 0) {
        // Some filesystems don't replace an existing file
        if (remove(path) != 0 || rename(temporaryPath, path) != 0) {
            remove(temporaryPath);
            return ERROR_RESOURCE;
        }
    }
    return ERROR_NONE;
}

} // namespace

bool development_trust_contains(const char* directory, const char* fingerprint) {
    if (strlen(fingerprint) != DEVELOPMENT_SERVER_FINGERPRINT_LENGTH) {
        return false;
    }
    mutex_lock(&fileMutex.handle);
    const bool contains = containsUnlocked(directory, fingerprint);
    mutex_unlock(&fileMutex.handle);
    return contains;
}

error_t development_trust_add(const char* directory, const char* fingerprint, const char* name) {
    if (strlen(fingerprint) != DEVELOPMENT_SERVER_FINGERPRINT_LENGTH) {
        return ERROR_INVALID_ARGUMENT;
    }
    mutex_lock(&fileMutex.handle);
    const error_t error = addUnlocked(directory, fingerprint, name);
    mutex_unlock(&fileMutex.handle);
    return error;
}

error_t development_trust_remove(const char* directory, const char* fingerprint) {
    if (strlen(fingerprint) != DEVELOPMENT_SERVER_FINGERPRINT_LENGTH) {
        return ERROR_INVALID_ARGUMENT;
    }
    mutex_lock(&fileMutex.handle);
    const error_t error = removeUnlocked(directory, fingerprint);
    mutex_unlock(&fileMutex.handle);
    return error;
}

error_t development_trust_for_each(const char* directory, DevelopmentTrustedClientVisitor visitor, void* context) {
    mutex_lock(&fileMutex.handle);
    forEachUnlocked(directory, visitor, context);
    mutex_unlock(&fileMutex.handle);
    return ERROR_NONE;
}
