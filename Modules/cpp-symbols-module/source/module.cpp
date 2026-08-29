// SPDX-License-Identifier: Apache-2.0
#include <cpp_symbols/module.h>

#include <bits/functexcept.h>
#include <cstddef>
#include <cstdint>
#include <new>
#include <string>

extern "C" {
    // cplusplus: compiler/runtime ABI support
    extern void* _Znwj(uint32_t size); // operator new(unsigned int)
    extern void _ZdlPvj(void* p, uint64_t size); // operator delete(void*, unsigned int)
    extern void __cxa_pure_virtual();
    // cxx_guards.cpp
    extern int __cxa_guard_acquire(void* pg);
    extern void __cxa_guard_release(void* pg) throw();
    extern void __cxa_guard_abort(void* pg) throw();
    extern void __cxa_guard_dummy(void);

    // stl: std::map / std::set red-black tree non-template helpers. We use the mangled names
    // directly (same pattern as the basic_string cold path below) to avoid ambiguity from the
    // overloaded const/non-const variants in stl_tree.h.
    void* _ZSt18_Rb_tree_decrementPSt18_Rb_tree_node_base(void*);
    void* _ZSt18_Rb_tree_incrementPSt18_Rb_tree_node_base(void*);
    void  _ZSt29_Rb_tree_insert_and_rebalancebPSt18_Rb_tree_node_baseS0_RS_(bool, void*, void*, void*);

    // string
    void _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcjPKcjj(void*, char*, unsigned int, char const*, unsigned int, unsigned int);
}

static const ModuleSymbol cpp_symbols_module_symbols[] = {
    // cplusplus
    DEFINE_MODULE_SYMBOL(_Znwj), // operator new(unsigned int)
    DEFINE_MODULE_SYMBOL(_ZdlPvj), // operator delete(void*, unsigned int)
    { "_ZSt7nothrow", (void*)&std::nothrow },
    DEFINE_MODULE_SYMBOL(__cxa_pure_virtual), // class-related, see https://arobenko.github.io/bare_metal_cpp/
    DEFINE_MODULE_SYMBOL(__cxa_guard_acquire),
    DEFINE_MODULE_SYMBOL(__cxa_guard_release),
    DEFINE_MODULE_SYMBOL(__cxa_guard_abort),
    DEFINE_MODULE_SYMBOL(__cxa_guard_dummy),
    // stl - Note: You have to use the mangled names here
    { "_ZSt17__throw_bad_allocv", (void*)&(std::__throw_bad_alloc) },
    { "_ZSt28__throw_bad_array_new_lengthv", (void*)&(std::__throw_bad_array_new_length) },
    { "_ZSt25__throw_bad_function_callv", (void*)&(std::__throw_bad_function_call) },
    { "_ZSt20__throw_length_errorPKc", (void*)&(std::__throw_length_error) },
    { "_ZSt19__throw_logic_errorPKc", (void*)&std::__throw_logic_error },
    { "_ZSt24__throw_out_of_range_fmtPKcz", (void*)&std::__throw_out_of_range_fmt },
    { "_ZSt20__throw_system_errori", (void*)&std::__throw_system_error },
    // stl - std::map / std::set (red-black tree internals)
    DEFINE_MODULE_SYMBOL(_ZSt18_Rb_tree_decrementPSt18_Rb_tree_node_base),
    DEFINE_MODULE_SYMBOL(_ZSt18_Rb_tree_incrementPSt18_Rb_tree_node_base),
    DEFINE_MODULE_SYMBOL(_ZSt29_Rb_tree_insert_and_rebalancebPSt18_Rb_tree_node_baseS0_RS_),
    // string - Note: You have to use the mangled names here
    DEFINE_MODULE_SYMBOL(_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcjPKcjj),
    MODULE_SYMBOL_TERMINATOR
};

extern "C" {

Module cpp_symbols_module = {
    .name = "cpp-symbols",
    .symbols = cpp_symbols_module_symbols
};

}
