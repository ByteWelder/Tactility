#ifndef NANOBAKE_NB_ASSERT_H
#define NANOBAKE_NB_ASSERT_H

#include <assert.h>
#include <esp_log.h>

#define NB_ASSERT(x, message) do {          \
        if (!(x)) {                         \
            ESP_LOGE("assert", message);    \
            _esp_error_check_failed(        \
                x,                          \
                __FILE__,                   \
                __LINE__,                   \
                __ASSERT_FUNC,              \
                #x                          \
            );                              \
        }                                   \
    } while(0)

#endif //NANOBAKE_NB_ASSERT_H
