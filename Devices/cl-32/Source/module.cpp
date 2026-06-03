#include <tactility/module.h>

extern "C" {

static error_t start() {
    // Empty for now
    return ERROR_NONE;
}

static error_t stop() {
    // Empty for now
    return ERROR_NONE;
}

struct Module cl_32_module = {
    .name = "cl-32",
    .start = start,
    .stop = stop
};

}
