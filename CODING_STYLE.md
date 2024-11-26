# C++ coding Style

The basic formatting rules are set in `.clang-format`. Use auto-formatting in your editor.

All code should target C++ language revision 17.

## Naming

### Files

Files are upper camel case.

- Files: `^[0-9a-zA-Z]+$`
- Directories: `^[0-9a-zA-Z]+$`
 
Example:
```c++
SomeFeature.cpp
SomeFeature.h
```

Private/internal headers are postfixed with `_i` before the file extension.
Like `SomeFeature_i.h`

### Folders

Project folders include:
- `Source` for source files and public header files
- `Private` for private header files
- `Include` for projects that require separate header files

Sources for a specific namespace must go into a folder with the lowercase name of that namespace.
The only exception to this is the root namespace, as this doesn't require its own subfolder.

### Function names

Names are lower camel case.

Example:

```c++
void getLimit() {
    // ...
}
```

### Preprocessor

Preprocessor functions are written in snake-case and prefixed with `tt_`, for example:

```c++
#define tt_assert(x) __tt_assert_internal(x)
```

### Type names

Consts are lower camel case with capital letters.

Typedefs for structs and datatype aliases are upper camel case.

Examples:

```c++
typedef uint32_t SomeAlias;

const SomeAlias myAlias;

typedef struct {
    // ...
} SomeStruct;
```
