#pragma once

#include "Manifest.h"
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
class App {
public:
    virtual ~App() {};
    virtual const Manifest& getManifest() const = 0;
    virtual _Nullable void* getData() const = 0;
    virtual void setData(void* data) = 0;
    virtual const Bundle& getParameters() const = 0;
    virtual Flags getFlags() const = 0;
};

}
