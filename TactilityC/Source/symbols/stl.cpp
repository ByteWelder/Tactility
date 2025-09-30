#include <symbols/stl.h>

#include <bits/functexcept.h>
#include <memory_resource>

const esp_elfsym stl_symbols[] = {
    // Note: You have to use the mangled names here
    { "_ZSt20__throw_length_errorPKc", (void*)&(std::__throw_length_error) },
    { "_ZSt28__throw_bad_array_new_lengthv", (void*)&(std::__throw_bad_array_new_length) },
    { "_ZNSt3pmr20get_default_resourceEv", (void*)&(std::pmr::get_default_resource) },
    // { "", (void*)&(std::) },
};
