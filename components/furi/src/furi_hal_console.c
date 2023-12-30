#include "furi_hal_console.h"
#include "furi_core.h"
#include "furi_string.h"

#include "esp_log.h" // TODO remove

#define TAG "FuriHalConsole"

typedef struct {
    bool alive;
    FuriHalConsoleTxCallback tx_callback;
    void* tx_callback_context;
} FuriHalConsole;

FuriHalConsole furi_hal_console = {
    .alive = false,
    .tx_callback = NULL,
    .tx_callback_context = NULL,
};

void furi_hal_console_init() {
    furi_hal_console.alive = true;
}

void furi_hal_console_enable() {
    furi_hal_console.alive = true;
}

void furi_hal_console_disable() {
    furi_hal_console.alive = false;
}

void furi_hal_console_set_tx_callback(FuriHalConsoleTxCallback callback, void* context) {
    FURI_CRITICAL_ENTER();
    furi_hal_console.tx_callback = callback;
    furi_hal_console.tx_callback_context = context;
    FURI_CRITICAL_EXIT();
}

void furi_hal_console_tx(const uint8_t* buffer, size_t buffer_size) {
    if (!furi_hal_console.alive) return;

    FURI_CRITICAL_ENTER();
    if (furi_hal_console.tx_callback) {
        furi_hal_console.tx_callback(buffer, buffer_size, furi_hal_console.tx_callback_context);
    }

    char safe_buffer[buffer_size + 1];
    memcpy(safe_buffer, buffer, buffer_size);
    safe_buffer[buffer_size] = 0;

    ESP_LOGI(TAG, "%s", safe_buffer);
    FURI_CRITICAL_EXIT();
}

void furi_hal_console_tx_with_new_line(const uint8_t* buffer, size_t buffer_size) {
    if (!furi_hal_console.alive) return;

    FURI_CRITICAL_ENTER();

    char safe_buffer[buffer_size + 1];
    memcpy(safe_buffer, buffer, buffer_size);
    safe_buffer[buffer_size] = 0;
    ESP_LOGI(TAG, "%s", safe_buffer);

    FURI_CRITICAL_EXIT();
}

void furi_hal_console_printf(const char format[], ...) {
    FuriString* string;
    va_list args;
    va_start(args, format);
    string = furi_string_alloc_vprintf(format, args);
    va_end(args);
    furi_hal_console_tx((const uint8_t*)furi_string_get_cstr(string), furi_string_size(string));
    furi_string_free(string);
}

void furi_hal_console_puts(const char* data) {
    furi_hal_console_tx((const uint8_t*)data, strlen(data));
}
