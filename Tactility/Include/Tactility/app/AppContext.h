#pragma once

#include <Tactility/Bundle.h>
#include <memory>

namespace tt::app {

// Forward declarations
class App;
class AppPaths;
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
    virtual std::unique_ptr<AppPaths> getPaths() const = 0;

    virtual std::shared_ptr<App> getApp() const = 0;
};


}
