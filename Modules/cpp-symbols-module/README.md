# cpp-symbols-module

Exports the C++ runtime/ABI symbols that side-loaded ELF apps need but that don't come from any
single library header - compiler-generated helpers (`operator new`/`delete`, vtable guard
variables) and libstdc++ internals that are normally only reachable through template
instantiation, not a plain function call. Apps still `#include <new>`/`<map>`/`<string>` etc.
directly; this module only makes sure the actual out-of-line definitions resolve when their ELF
is loaded into the firmware.

ESP32-only: several of these symbols are mangled for a 32-bit ABI (`j` = `unsigned int`, used
here to represent `size_t`). On a 64-bit host those would mangle differently (e.g. `_Znwm`
instead of `_Znwj`), so this module doesn't build for the POSIX simulator.

## Supported symbols

### Compiler/runtime ABI support

- `operator new(unsigned int)` / `operator delete(void*, unsigned int)` (`_Znwj` / `_ZdlPvj`)
- `std::nothrow`
- `__cxa_pure_virtual` - called through a pure-virtual slot before a derived class's vtable is
  fully constructed; see [Bare metal C++](https://arobenko.github.io/bare_metal_cpp/).
- `__cxa_guard_acquire` / `__cxa_guard_release` / `__cxa_guard_abort` / `__cxa_guard_dummy` -
  thread-safe one-time initialization of function-local `static` variables.

### libstdc++ exception helpers

Out-of-line `std::__throw_*` functions libstdc++ headers call instead of throwing directly, to
keep the throw site small:

- `std::__throw_bad_alloc`
- `std::__throw_bad_array_new_length`
- `std::__throw_bad_function_call`
- `std::__throw_length_error`
- `std::__throw_logic_error`
- `std::__throw_out_of_range_fmt`
- `std::__throw_system_error`

### `std::map` / `std::set` (red-black tree internals)

Non-template helpers shared by every `std::map`/`std::set` instantiation:

- `std::_Rb_tree_increment` / `std::_Rb_tree_decrement`
- `std::_Rb_tree_insert_and_rebalance`

### `std::string`

- `basic_string::_M_replace_cold` - the rarely-taken slow path of `std::string::replace`,
  split out of the header-inlined fast path.

## Adding a new symbol

1. Find the mangled name (`nm`/`c++filt`, or the linker's "undefined reference" error from an
   app build).
2. Add an `extern "C"` declaration for it in `source/module.cpp` if the name isn't already a
   valid identifier you can reference directly (mangled names usually are, e.g. `_ZSt19...`).
3. Add a `DEFINE_MODULE_SYMBOL(...)` entry (or a manual `{ "mangled_name", (void*)&expr }` pair
   when the address isn't reachable through the mangled identifier itself, e.g. `std::nothrow`
   or the `__throw_*` functions).

## License

This module is licensed under the [Apache v2.0](LICENSE-Apache-2.0.md) license.
