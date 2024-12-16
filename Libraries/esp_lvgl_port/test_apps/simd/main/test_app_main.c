/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "unity.h"
#include "unity_test_utils.h"
#include "lv_fill_common.h"

#define TEST_MEMORY_LEAK_THRESHOLD (300)

void app_main(void)
{

    // ______  _____ ______   _               _
    // |  _  \/  ___|| ___ \ | |             | |
    // | | | |\ `--. | |_/ / | |_   ___  ___ | |_
    // | | | | `--. \|  __/  | __| / _ \/ __|| __|
    // | |/ / /\__/ /| |     | |_ |  __/\__ \| |_
    // |___/  \____/ \_|      \__| \___||___/ \__|

    printf("______  _____ ______   _               _   \r\n");
    printf("|  _  \\/  ___|| ___ \\ | |             | |  \r\n");
    printf("| | | |\\ `--. | |_/ / | |_   ___  ___ | |_ \r\n");
    printf("| | | | `--. \\|  __/  | __| / _ \\/ __|| __|\r\n");
    printf("| |/ / /\\__/ /| |     | |_ |  __/\\__ \\| |_ \r\n");
    printf("|___/  \\____/ \\_|      \\__| \\___||___/ \\__|\r\n");


    UNITY_BEGIN();
    unity_run_menu();
    UNITY_END();
}

/* setUp runs before every test */
void setUp(void)
{
    // Check for memory leaks
    unity_utils_set_leak_level(TEST_MEMORY_LEAK_THRESHOLD);
    unity_utils_record_free_mem();
}

/* tearDown runs after every test */
void tearDown(void)
{
    // Evaluate memory leaks
    unity_utils_evaluate_leaks();
}
