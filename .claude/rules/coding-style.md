# Coding Style

Two conventions coexist; which one to use depends on the project layer:

- **C code** (TactilityKernel, drivers): `lower_snake_case` for files, functions, variables. `UpperCamelCase` for types. Files in `source/`, `include/`, `private/` directories.
- **C++ code** (Tactility, apps, services): `UpperCamelCase` for files and types. `lowerCamelCase` for functions. Files in `Source/`, `Include/`, `Private/` directories.

For projects that emit C headers and have a C++ implementation file: the internal C++ function naming should be snake_case.

Formatting is enforced by `.clang-format` (LLVM-based, 4-space indent, no column limit).
Never throw exceptions — use return types for error handling. Use `enum class` over plain `enum` when writing C++ code.
Do not add redundant null checks for parameters with an explicit non-null precondition.

Code Comments:

- Should be as short as possible, leaving only important context.
- Should avoid explaining what the code does, unless the code complexity is high enough to warrant an explanation.
- Must avoid explaining how the code was before, or how it was changed.
- Should explain why code is implemented.
- Should be as brief as possible without losing critical information.
- Should avoid explaining what was not implemented.
- Should avoid referring to designs of other subsystems.
- Must avoid interjections: avoid hyphens or braces to interject. If interjections provide crucial info, use Doxygen entity/anchor references like:
/**
 * A dedicated completion \signal for one app instance's task.
 * Whichever \side finishes with it last is the one that deletes `semaphore` and frees this struct.
 *
 * \signal Not the task's shared default FreeRTOS notification, which app_event.cpp's AppEventSubscription also uses.
 *     An unrelated event delivered to the same task could otherwise unblock a waiter early.
 * \side The exiting task or a concurrent app_scheduler_stop() that found the entry in time and is waiting on `semaphore`.
 */
```


