#include "hal_console.h"
#include "tactility_core.h"
#include "tt_string.h"

#include "esp_log.h" // TODO remove

#define TAG "FuriHalConsole"

typedef struct {
    bool alive;
    HalConsoleTxCallback tx_callback;
    void* tx_callback_context;
} HalConsole;

static HalConsole hal_console = {
    .alive = false,
    .tx_callback = NULL,
    .tx_callback_context = NULL,
};

void tt_hal_console_init() {
    hal_console.alive = true;
}

void tt_hal_console_enable() {
    hal_console.alive = true;
}

void tt_hal_console_disable() {
    hal_console.alive = false;
}

void tt_hal_console_set_tx_callback(HalConsoleTxCallback callback, void* context) {
    TT_CRITICAL_ENTER();
    hal_console.tx_callback = callback;
    hal_console.tx_callback_context = context;
    TT_CRITICAL_EXIT();
}

void tt_hal_console_tx(const uint8_t* buffer, size_t buffer_size) {
    if (!hal_console.alive) return;

    TT_CRITICAL_ENTER();
    if (hal_console.tx_callback) {
        hal_console.tx_callback(buffer, buffer_size, hal_console.tx_callback_context);
    }

    char safe_buffer[buffer_size + 1];
    memcpy(safe_buffer, buffer, buffer_size);
    safe_buffer[buffer_size] = 0;

    ESP_LOGI(TAG, "%s", safe_buffer);
    TT_CRITICAL_EXIT();
}

void tt_hal_console_tx_with_new_line(const uint8_t* buffer, size_t buffer_size) {
    if (!hal_console.alive) return;

    TT_CRITICAL_ENTER();

    char safe_buffer[buffer_size + 1];
    memcpy(safe_buffer, buffer, buffer_size);
    safe_buffer[buffer_size] = 0;
    ESP_LOGI(TAG, "%s", safe_buffer);

    TT_CRITICAL_EXIT();
}

void tt_hal_console_printf(const char format[], ...) {
    TtString* string;
    va_list args;
    va_start(args, format);
    string = tt_string_alloc_vprintf(format, args);
    va_end(args);
    tt_hal_console_tx((const uint8_t*)tt_string_get_cstr(string), tt_string_size(string));
    tt_string_free(string);
}

void tt_hal_console_puts(const char* data) {
    tt_hal_console_tx((const uint8_t*)data, strlen(data));
}
