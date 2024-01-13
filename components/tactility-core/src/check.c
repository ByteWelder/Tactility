#include "check.h"
#include "core_defines.h"

#include "hal_console.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>

static void __tt_put_uint32_as_text(uint32_t data) {
    char tmp_str[] = "-2147483648";
    itoa(data, tmp_str, 10);
    tt_hal_console_puts(tmp_str);
}

static void __tt_print_stack_info() {
    tt_hal_console_puts("\r\n\tstack watermark: ");
    __tt_put_uint32_as_text(uxTaskGetStackHighWaterMark(NULL) * 4);
}

static void __tt_print_heap_info() {
    tt_hal_console_puts("\r\n\theap total: ");
    __tt_put_uint32_as_text(heap_caps_get_total_size(MALLOC_CAP_DEFAULT));
    tt_hal_console_puts("\r\n\theap free: ");
    __tt_put_uint32_as_text(heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
    tt_hal_console_puts("\r\n\theap min free: ");
    __tt_put_uint32_as_text(heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
}

static void __tt_print_name(bool isr) {
    if (isr) {
        tt_hal_console_puts("[ISR ");
        __tt_put_uint32_as_text(__get_IPSR());
        tt_hal_console_puts("] ");
    } else {
        const char* name = pcTaskGetName(NULL);
        if (name == NULL) {
            tt_hal_console_puts("[main] ");
        } else {
            tt_hal_console_puts("[");
            tt_hal_console_puts(name);
            tt_hal_console_puts("] ");
        }
    }
}

TT_NORETURN void __tt_crash_implementation() {
    __disable_irq();
    //    GET_MESSAGE_AND_STORE_REGISTERS();

    bool isr = TT_IS_IRQ_MODE();

    tt_hal_console_puts("\r\n\033[0;31m[CRASH]");
    __tt_print_name(isr);

    if (!isr) {
        __tt_print_stack_info();
    }
    __tt_print_heap_info();

    // Check if debug enabled by DAP
    // https://developer.arm.com/documentation/ddi0403/d/Debug-Architecture/ARMv7-M-Debug/Debug-register-support-in-the-SCS/Debug-Halting-Control-and-Status-Register--DHCSR?lang=en
//    bool debug = CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk;
#ifdef TT_NDEBUG
    if (debug) {
#endif
        tt_hal_console_puts("\r\nSystem halted. Connect debugger for more info\r\n");
        tt_hal_console_puts("\033[0m\r\n");
        //        tt_hal_debug_enable();

        esp_system_abort("crash");
#ifdef TT_NDEBUG
    } else {
        uint32_t ptr = (uint32_t)__tt_check_message;
        if (ptr < FLASH_BASE || ptr > (FLASH_BASE + FLASH_SIZE)) {
            ptr = (uint32_t) "Check serial logs";
        }
        tt_hal_rtc_set_fault_data(ptr);
        tt_hal_console_puts("\r\nRebooting system.\r\n");
        tt_hal_console_puts("\033[0m\r\n");
        esp_system_abort("crash");
    }
#endif
    __builtin_unreachable();
}
TT_NORETURN void __tt_halt_implementation() {
    __disable_irq();

    bool isr = TT_IS_IRQ_MODE();

    tt_hal_console_puts("\r\n\033[0;31m[HALT]");
    __tt_print_name(isr);
    tt_hal_console_puts("\r\nSystem halted. Bye-bye!\r\n");
    tt_hal_console_puts("\033[0m\r\n");

    __builtin_unreachable();
}
