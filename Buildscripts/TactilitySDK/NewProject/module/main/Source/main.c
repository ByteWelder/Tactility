#include <tactility/error.h>
#include <tactility/module.h>

#include <stdio.h>

static error_t start(void) {
    printf("Module started\n");
    return ERROR_NONE;
}

static error_t stop(void) {
    printf("Module stopped\n");
    return ERROR_NONE;
}

static struct Module module = {
    .name = "example-module",
    .start = start,
    .stop = stop,
    .drivers = NULL,
    .symbols = NULL,
    .internal = NULL,
};

// The entry point of a module returns its struct Module instead of an exit code.
// The asm label keeps the symbol name of main while allowing another return type.
// The build renames main to app_main for the ELF loader, which is why the label goes through a macro.
#define ENTRY_NAME_INNER(name) #name
#define ENTRY_NAME(name) ENTRY_NAME_INNER(name)
struct Module* module_main(int argc, char* argv[]) __asm__(ENTRY_NAME(main));

struct Module* module_main(int argc, char* argv[]) {
    return &module;
}
