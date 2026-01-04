#include <private/elf_symbol.h>
#include <symbols/string.h>

#include <string>

extern "C" void _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcjPKcjj(void*, char*, unsigned int, char const*, unsigned int, unsigned int);

const esp_elfsym string_symbols[] = {
    // Note: You have to use the mangled names here
    { "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcjPKcjj", (void*)&_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcjPKcjj},
    // { "", (void*)&(std::) },
    // delimiter
    ESP_ELFSYM_END
};
