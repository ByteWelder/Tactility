#include <private/elf_symbol.h>
#include <cstddef>

#include <symbols/stl.h>

#include <bits/functexcept.h>

const esp_elfsym stl_symbols[] = {
    // Note: You have to use the mangled names here
    { "_ZSt17__throw_bad_allocv", (void*)&(std::__throw_bad_alloc) },
    { "_ZSt28__throw_bad_array_new_lengthv", (void*)&(std::__throw_bad_array_new_length) },
    { "_ZSt25__throw_bad_function_callv", (void*)&(std::__throw_bad_function_call) },
    { "_ZSt20__throw_length_errorPKc", (void*)&(std::__throw_length_error) },
    // { "", (void*)&(std::) },
    // delimiter
    ESP_ELFSYM_END
};
