#include <stdio.h>
#include "ohos_init.h"
#include "cmsis_os2.h"

static void HelloTask(void *arg)
{
    (void)arg;
    printf("[HELLO] Hello World from test_simple!\n");
    
    while (1) {
        printf("[HELLO] Test simple app is running...\n");
        osDelay(5000);
    }
}

static void HelloWorldDemo(void)
{
    osThreadAttr_t attr;
    attr.name = "HelloTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 0x1000;
    attr.priority = osPriorityNormal;

    if (osThreadNew(HelloTask, NULL, &attr) == NULL) {
        printf("[HELLO] Failed to create HelloTask!\n");
    } else {
        printf("[HELLO] HelloTask created successfully!\n");
    }
}

APP_FEATURE_INIT(HelloWorldDemo);
