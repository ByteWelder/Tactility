# Architecture: Hardware Abstraction Layer

## Driver

A driver generally consists of:
- Registration of driver in parent module (optional, but desirable)
- YAML bindings in the `bindings/` folder
- An `#include` that is used in the `.dts` file. The include is in `[projectname]/bindings/[drivername].h`
- The driver implementation: a `.cpp` and `.h` file. The implementation is C++, but the header exposes pure C functions. C implementations are allowed, but C++ is preferred.

Drivers are part of a kernel module.

Modules with drivers can be stored in:
- TactilityKernel
- A subproject in `Platforms` folder
- A subproject in `Devices` folder
- A subproject in `Drivers` folder

## Kernel Modules

Kernel module names are lower case and postfixed with `-module`.

Projects that are kernel modules:

1. Declare a `struct Module`
2. Contain a `devicetree.yaml` file that declares a list of dependencies (for parsing the devicetree) and specifies the bindings folder that contains the drivers' YAML definitions. For example:
```yaml
dependencies:
  - TactilityKernel
bindings: bindings
```
