#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include <cassert>

#include "FreeRTOS.h"
#include "task.h"

typedef struct {
    int argc;
    char** argv;
    int result;
} TestTaskData;

void test_task(void* parameter) {
    auto* data = (TestTaskData*)parameter;

    doctest::Context context;

    context.applyCommandLine(data->argc, data->argv);

    // overrides
    context.setOption("no-breaks", true); // don't break in the debugger when assertions fail

    data->result = context.run();

    if (context.shouldExit()) { // important - query flags (and --exit) rely on the user doing this
        vTaskEndScheduler();
    }

    vTaskDelete(nullptr);
}

int main(int argc, char** argv) {
    TestTaskData data = {
        .argc = argc,
        .argv = argv,
        .result = 0
    };

    BaseType_t task_result = xTaskCreate(
        test_task,
        "test_task",
        8192,
        &data,
        1,
        nullptr
    );
    assert(task_result == pdPASS);

    vTaskStartScheduler();
}
