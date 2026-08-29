// SPDX-License-Identifier: Apache-2.0
#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

#include <tactility/module.h>

#include <esp_event.h>

// GCC soft-float / compiler-rt helpers an app may need but can't reach through a header -
// reference: https://gcc.gnu.org/onlinedocs/gccint/Soft-float-library-routines.html
// ESP32P4 (RISC-V, native single-precision FPU) only needs the double-precision + 64-bit-div
// subset; every other (Xtensa) target needs the full single- and double-precision set.
extern "C" {
#ifndef CONFIG_IDF_TARGET_ESP32P4
    extern float __addsf3(float a, float b);
    extern double __adddf3(double a, double b);
    extern float __subsf3(float a, float b);
    extern double __subdf3(double a, double b);
    extern float __mulsf3(float a, float b);
    extern double __muldf3(double a, double b);
    extern float __divsf3(float a, float b);
    extern double __divdf3(double a, double b);
    extern float __negsf2(float a);
    extern double __negdf2(double a);
    extern double __extendsfdf2(float a);
    extern float __truncdfsf2(double a);
    extern int __fixsfsi(float a);
    extern int __fixdfsi(double a);
    extern long __fixsfdi(float a);
    extern long __fixdfdi(double a);
    extern unsigned int __fixunssfsi(float a);
    extern unsigned int __fixunsdfsi(double a);
    extern unsigned long __fixunssfdi(float a);
    extern unsigned long __fixunsdfdi(double a);
    extern float __floatsisf(int i);
    extern double __floatsidf(int i);
    extern float __floatdisf(long i);
    extern double __floatdidf(long i);
    extern float __floatunsisf(unsigned int i);
    extern double __floatunsidf(unsigned int i);
    extern float __floatundisf(unsigned long i);
    extern double __floatundidf(unsigned long i);
    float __powisf2(float a, int b);
    double __powidf2(double a, int b);
    int __cmpdf2(double a, double b);
    int __unordsf2(float a, float b);
    int __unorddf2(double a, double b);
    int __eqsf2(float a, float b);
    int __eqdf2(double a, double b);
    int __nesf2(float a, float b);
    int __nedf2(double a, double b);
    int __gesf2(float a, float b);
    int __gedf2(double a, double b);
    int __ltsf2(float a, float b);
    int __ltdf2(double a, double b);
    int __lesf2(float a, float b);
    int __ledf2(double a, double b);
    int __gtsf2(float a, float b);
    int __gtdf2(double a, double b);
    // GCC integer arithmetic helpers (needed on 32-bit targets for 64-bit ops)
    long long __divdi3(long long a, long long b);
    long long __moddi3(long long a, long long b);
    unsigned long long __udivdi3(unsigned long long a, unsigned long long b);
    unsigned long long __umoddi3(unsigned long long a, unsigned long long b);
#else
    extern double __adddf3(double a, double b);
    extern double __subdf3(double a, double b);
    extern double __muldf3(double a, double b);
    extern double __divdf3(double a, double b);
    extern double __negdf2(double a);
    extern double __extendsfdf2(float a);
    extern float __truncdfsf2(double a);
    extern int __fixdfsi(double a);
    extern long __fixdfdi(double a);
    extern unsigned int __fixunsdfsi(double a);
    extern unsigned long __fixunssfdi(float a);
    extern unsigned long __fixunsdfdi(double a);
    extern double __floatsidf(int i);
    extern float __floatdisf(long i);
    extern double __floatdidf(long i);
    extern double __floatunsidf(unsigned int i);
    extern float __floatundisf(unsigned long i);
    extern double __floatundidf(unsigned long i);
    float __powisf2(float a, int b);
    double __powidf2(double a, int b);
    int __cmpdf2(double a, double b);
    int __unorddf2(double a, double b);
    int __eqdf2(double a, double b);
    int __nedf2(double a, double b);
    int __gedf2(double a, double b);
    int __ltdf2(double a, double b);
    int __ledf2(double a, double b);
    int __gtdf2(double a, double b);
    // GCC integer/bitwise helpers (compiler-rt)
    int __clzsi2(unsigned int x);
    // GCC 64-bit integer arithmetic helpers (needed for 64-bit div on 32-bit RISC-V)
    long long __divdi3(long long a, long long b);
    unsigned long long __udivdi3(unsigned long long a, unsigned long long b);
#endif
}

extern "C" {

const ModuleSymbol platform_esp32_symbols[] = {
    DEFINE_MODULE_SYMBOL(esp_event_loop_create),
    DEFINE_MODULE_SYMBOL(esp_event_loop_delete),
    DEFINE_MODULE_SYMBOL(esp_event_loop_create_default),
    DEFINE_MODULE_SYMBOL(esp_event_loop_delete_default),
    DEFINE_MODULE_SYMBOL(esp_event_loop_run),
    DEFINE_MODULE_SYMBOL(esp_event_handler_register),
    DEFINE_MODULE_SYMBOL(esp_event_handler_register_with),
    DEFINE_MODULE_SYMBOL(esp_event_handler_instance_register_with),
    DEFINE_MODULE_SYMBOL(esp_event_handler_instance_register),
    DEFINE_MODULE_SYMBOL(esp_event_handler_unregister),
    DEFINE_MODULE_SYMBOL(esp_event_handler_unregister_with),
    DEFINE_MODULE_SYMBOL(esp_event_handler_instance_unregister_with),
    DEFINE_MODULE_SYMBOL(esp_event_handler_instance_unregister),
    DEFINE_MODULE_SYMBOL(esp_event_post),
    DEFINE_MODULE_SYMBOL(esp_event_post_to),
    DEFINE_MODULE_SYMBOL(esp_event_isr_post),
    DEFINE_MODULE_SYMBOL(esp_event_isr_post_to),
#ifndef CONFIG_IDF_TARGET_ESP32P4
    DEFINE_MODULE_SYMBOL(__addsf3),
    DEFINE_MODULE_SYMBOL(__adddf3),
    DEFINE_MODULE_SYMBOL(__subsf3),
    DEFINE_MODULE_SYMBOL(__subdf3),
    DEFINE_MODULE_SYMBOL(__mulsf3),
    DEFINE_MODULE_SYMBOL(__muldf3),
    DEFINE_MODULE_SYMBOL(__divsf3),
    DEFINE_MODULE_SYMBOL(__divdf3),
    DEFINE_MODULE_SYMBOL(__negsf2),
    DEFINE_MODULE_SYMBOL(__negdf2),
    DEFINE_MODULE_SYMBOL(__extendsfdf2),
    DEFINE_MODULE_SYMBOL(__truncdfsf2),
    DEFINE_MODULE_SYMBOL(__fixsfsi),
    DEFINE_MODULE_SYMBOL(__fixdfsi),
    DEFINE_MODULE_SYMBOL(__fixsfdi),
    DEFINE_MODULE_SYMBOL(__fixdfdi),
    DEFINE_MODULE_SYMBOL(__fixunssfsi),
    DEFINE_MODULE_SYMBOL(__fixunsdfsi),
    DEFINE_MODULE_SYMBOL(__fixunssfdi),
    DEFINE_MODULE_SYMBOL(__fixunsdfdi),
    DEFINE_MODULE_SYMBOL(__floatsisf),
    DEFINE_MODULE_SYMBOL(__floatsidf),
    DEFINE_MODULE_SYMBOL(__floatdisf),
    DEFINE_MODULE_SYMBOL(__floatdidf),
    DEFINE_MODULE_SYMBOL(__floatunsisf),
    DEFINE_MODULE_SYMBOL(__floatunsidf),
    DEFINE_MODULE_SYMBOL(__floatundisf),
    DEFINE_MODULE_SYMBOL(__floatundidf),
    DEFINE_MODULE_SYMBOL(__powisf2),
    DEFINE_MODULE_SYMBOL(__powidf2),
    DEFINE_MODULE_SYMBOL(__unordsf2),
    DEFINE_MODULE_SYMBOL(__unorddf2),
    DEFINE_MODULE_SYMBOL(__eqsf2),
    DEFINE_MODULE_SYMBOL(__eqdf2),
    DEFINE_MODULE_SYMBOL(__nesf2),
    DEFINE_MODULE_SYMBOL(__nedf2),
    DEFINE_MODULE_SYMBOL(__gesf2),
    DEFINE_MODULE_SYMBOL(__gedf2),
    DEFINE_MODULE_SYMBOL(__ltsf2),
    DEFINE_MODULE_SYMBOL(__ltdf2),
    DEFINE_MODULE_SYMBOL(__lesf2),
    DEFINE_MODULE_SYMBOL(__ledf2),
    DEFINE_MODULE_SYMBOL(__gtsf2),
    DEFINE_MODULE_SYMBOL(__gtdf2),
    DEFINE_MODULE_SYMBOL(__divdi3),
    DEFINE_MODULE_SYMBOL(__moddi3),
    DEFINE_MODULE_SYMBOL(__udivdi3),
    DEFINE_MODULE_SYMBOL(__umoddi3),
#else
    DEFINE_MODULE_SYMBOL(__adddf3),
    DEFINE_MODULE_SYMBOL(__subdf3),
    DEFINE_MODULE_SYMBOL(__muldf3),
    DEFINE_MODULE_SYMBOL(__divdf3),
    DEFINE_MODULE_SYMBOL(__negdf2),
    DEFINE_MODULE_SYMBOL(__extendsfdf2),
    DEFINE_MODULE_SYMBOL(__truncdfsf2),
    DEFINE_MODULE_SYMBOL(__fixdfsi),
    DEFINE_MODULE_SYMBOL(__fixdfdi),
    DEFINE_MODULE_SYMBOL(__fixunsdfsi),
    DEFINE_MODULE_SYMBOL(__fixunssfdi),
    DEFINE_MODULE_SYMBOL(__fixunsdfdi),
    DEFINE_MODULE_SYMBOL(__floatsidf),
    DEFINE_MODULE_SYMBOL(__floatdisf),
    DEFINE_MODULE_SYMBOL(__floatdidf),
    DEFINE_MODULE_SYMBOL(__floatunsidf),
    DEFINE_MODULE_SYMBOL(__floatundisf),
    DEFINE_MODULE_SYMBOL(__floatundidf),
    DEFINE_MODULE_SYMBOL(__powisf2),
    DEFINE_MODULE_SYMBOL(__powidf2),
    DEFINE_MODULE_SYMBOL(__unorddf2),
    DEFINE_MODULE_SYMBOL(__eqdf2),
    DEFINE_MODULE_SYMBOL(__nedf2),
    DEFINE_MODULE_SYMBOL(__gedf2),
    DEFINE_MODULE_SYMBOL(__ltdf2),
    DEFINE_MODULE_SYMBOL(__ledf2),
    DEFINE_MODULE_SYMBOL(__gtdf2),
    DEFINE_MODULE_SYMBOL(__clzsi2),
    DEFINE_MODULE_SYMBOL(__divdi3),
    DEFINE_MODULE_SYMBOL(__udivdi3),
#endif
    MODULE_SYMBOL_TERMINATOR
};

}
