#include "check.h"
#include "furi_core_defines.h"

#include "furi_hal_console.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdlib.h>

static void __furi_put_uint32_as_text(uint32_t data) {
    char tmp_str[] = "-2147483648";
    itoa(data, tmp_str, 10);
    furi_hal_console_puts(tmp_str);
}

static void __furi_print_stack_info() {
    furi_hal_console_puts("\r\n\tstack watermark: ");
    __furi_put_uint32_as_text(uxTaskGetStackHighWaterMark(NULL) * 4);
}

static void __furi_print_heap_info() {
    furi_hal_console_puts("\r\n\theap total: ");
    __furi_put_uint32_as_text(heap_caps_get_total_size(MALLOC_CAP_DEFAULT));
    furi_hal_console_puts("\r\n\theap free: ");
    __furi_put_uint32_as_text(heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
    furi_hal_console_puts("\r\n\theap min free: ");
    __furi_put_uint32_as_text(heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
}

static void __furi_print_name(bool isr) {
    if (isr) {
        furi_hal_console_puts("[ISR ");
        __furi_put_uint32_as_text(__get_IPSR());
        furi_hal_console_puts("] ");
    } else {
        const char* name = pcTaskGetName(NULL);
        if (name == NULL) {
            furi_hal_console_puts("[main] ");
        } else {
            furi_hal_console_puts("[");
            furi_hal_console_puts(name);
            furi_hal_console_puts("] ");
        }
    }
}

FURI_NORETURN void __furi_crash_implementation() {
    __disable_irq();
    //    GET_MESSAGE_AND_STORE_REGISTERS();

    bool isr = FURI_IS_IRQ_MODE();

    furi_hal_console_puts("\r\n\033[0;31m[CRASH]");
    __furi_print_name(isr);

    if (!isr) {
        __furi_print_stack_info();
    }
    __furi_print_heap_info();

    // Check if debug enabled by DAP
    // https://developer.arm.com/documentation/ddi0403/d/Debug-Architecture/ARMv7-M-Debug/Debug-register-support-in-the-SCS/Debug-Halting-Control-and-Status-Register--DHCSR?lang=en
//    bool debug = CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk;
#ifdef FURI_NDEBUG
    if (debug) {
#endif
        furi_hal_console_puts("\r\nSystem halted. Connect debugger for more info\r\n");
        furi_hal_console_puts("\033[0m\r\n");
        //        furi_hal_debug_enable();

        esp_system_abort("crash");
#ifdef FURI_NDEBUG
    } else {
        uint32_t ptr = (uint32_t)__furi_check_message;
        if (ptr < FLASH_BASE || ptr > (FLASH_BASE + FLASH_SIZE)) {
            ptr = (uint32_t) "Check serial logs";
        }
        furi_hal_rtc_set_fault_data(ptr);
        furi_hal_console_puts("\r\nRebooting system.\r\n");
        furi_hal_console_puts("\033[0m\r\n");
        esp_system_abort("crash");
    }
#endif
    __builtin_unreachable();
}
FURI_NORETURN void __furi_halt_implementation() {
    __disable_irq();

    bool isr = FURI_IS_IRQ_MODE();

    furi_hal_console_puts("\r\n\033[0;31m[HALT]");
    __furi_print_name(isr);
    furi_hal_console_puts("\r\nSystem halted. Bye-bye!\r\n");
    furi_hal_console_puts("\033[0m\r\n");

    __builtin_unreachable();
}
