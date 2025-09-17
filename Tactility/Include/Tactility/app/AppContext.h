#pragma once

#include <Tactility/Bundle.h>
#include <memory>

namespace tt::app {

// Forward declarations
class App;
class Paths;
struct AppManifest;
enum class Result;

typedef union {
    struct {
        bool hideStatusbar : 1;
    };
    unsigned char flags;
} Flags;

/**
 * The public representation of an application instance.
 * @warning Do not store references or pointers to these! You can retrieve them via the service registry.
 */
class AppContext {

protected:

    virtual ~AppContext() = default;

public:

    virtual const AppManifest& getManifest() const = 0;
    virtual std::shared_ptr<const Bundle> getParameters() const = 0;
    virtual std::unique_ptr<Paths> getPaths() const = 0;

    virtual std::shared_ptr<App> getApp() const = 0;
};

class Paths {

public:

    Paths() = default;
    virtual ~Paths() = default;

    /**
     * Returns the directory path for the data location for an app.
     * The data directory is intended to survive OS upgrades.
     * The path will not end with a "/".
     */
    virtual std::string getDataDirectory() const = 0;

    /**
     * @see getDataDirectory(), but with LVGL prefix.
     */
    virtual std::string getDataDirectoryLvgl() const = 0;

    /**
     * Returns the full path for an entry inside the data location for an app.
     * The data directory is intended to survive OS upgrades.
     * Configuration data should be stored here.
     * @param[in] childPath the path without a "/" prefix
     */
    virtual std::string getDataPath(const std::string& childPath) const = 0;

    /**
     * @see getDataPath(), but with LVGL prefix.
     */
    virtual std::string getDataPathLvgl(const std::string& childPath) const = 0;

    /**
     * Returns the directory path for the system location for an app.
     * The system directory is not intended to survive OS upgrades.
     * You should not store configuration data here.
     * The path will not end with a "/".
     * This is mainly used for core apps (system/boot/settings type).
     */
    virtual std::string getSystemDirectory() const = 0;

    /**
     * @see getSystemDirectory(), but with LVGL prefix.
     */
    virtual std::string getSystemDirectoryLvgl() const = 0;

    /**
     * Returns the full path for an entry inside the system location for an app.
     * The data directory is not intended to survive OS upgrades.
     * You should not store configuration data here.
     * This is mainly used for core apps (system/boot/settings type).
     * @param[in] childPath the path without a "/" prefix
     */
    virtual std::string getSystemPath(const std::string& childPath) const = 0;

    /**
     * @see getSystemPath(), but with LVGL prefix.
     */
    virtual std::string getSystemPathLvgl(const std::string& childPath) const = 0;
};

}
