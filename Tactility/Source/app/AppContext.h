#pragma once

#include "AppManifest.h"
#include "Bundle.h"

namespace tt::app {

typedef union {
    struct {
        bool showStatusbar : 1;
    };
    unsigned char flags;
} Flags;

/**
 * A limited representation of the application instance.
 * Do not store references or pointers to these!
 */
class AppContext {

protected:

    virtual ~AppContext() = default;

public:

    [[nodiscard]] virtual const AppManifest& getManifest() const = 0;
    [[nodiscard]] virtual _Nullable void* getData() const = 0;
    virtual void setData(void* data) = 0;
    [[nodiscard]] virtual const Bundle& getParameters() const = 0;
    [[nodiscard]] virtual Flags getFlags() const = 0;
    virtual void setResult(Result result) = 0;
    virtual void setResult(Result result, const Bundle& bundle)= 0;
    [[nodiscard]] virtual bool hasResult() const = 0;
};

}
