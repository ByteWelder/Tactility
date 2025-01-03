#pragma once

#include "AppManifest.h"
#include "Bundle.h"
#include <memory>

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

    virtual const AppManifest& getManifest() const = 0;
    virtual std::shared_ptr<void> _Nullable getData() const = 0;
    virtual void setData(std::shared_ptr<void> data) = 0;
    virtual std::shared_ptr<const Bundle> getParameters() const = 0;
    virtual Flags getFlags() const = 0;
    virtual void setResult(Result result) = 0;
    virtual void setResult(Result result, std::shared_ptr<const Bundle> bundle)= 0;
    virtual bool hasResult() const = 0;
};

}
