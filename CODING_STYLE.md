# C coding Style

The basic formatting rules are set in `.clang-format`. Use auto-formatting in your editor.

All code should target C language revision C11/C17.

## Naming

### Files

Files are snake-case.

- Files: `^[0-9a-z_]+$`
- Directories: `^[0-9a-z_]+$`
 
Example:
```c
some_feature.c
some_feature.h
```

Private/internal headers are postfixed with `_i` before the file extension.
Like `some_feature_i.h`

### Function names

Names are snake-case.

Public functions are prefixed with `tt_` for `tactility-core` and `tactility` projects.
Internal/static functions don't have prefix requirements, but prefixes are allowed.

Public functions have the feature name after `tt_`.

If a feature has setters or getters, it's added after the feature name part.

Example:

```c
void tt_feature_get_name() {
    // ...
}
```

Function names that allocate or free memory should end in `_alloc` and `_free`.

### Type names

Consts are snake-case with capital letters.

Typedefs for structs and datatype aliases are PascalCase.
Examples:

```c
typedef uint32_t SomeAlias;

typedef struct {
    // ...
} SomeStruct;
```

### Internal struct with public handle

When you have a `struct` data type that is private and you want to expose a handle (pointer),
append the internal name with `Data` like this:

**feature.c**
```c
typedef struct {
    // ...
} MutexData;
```

**feature.h**
```c
typedef void* Mutex;
```
