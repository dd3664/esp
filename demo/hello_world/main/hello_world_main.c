/****************************************************************************************************/
/*                                           INCLUDE                                                */
/****************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/****************************************************************************************************/
/*                                           DEFINES                                                */
/****************************************************************************************************/
#define CPU0    0
#define CPU1    1

/****************************************************************************************************/
/*                                           VARIABLES                                              */
/****************************************************************************************************/

/****************************************************************************************************/
/*                                       STATIC FUNCTIONS                                           */
/****************************************************************************************************/
static void test_task1(void *pvParameters)
{
    int core_id;
    while (1)
    {
        core_id = xPortGetCoreID();
        printf("[%s:%d] This is a test of dongdong!, running in core %d\n", __func__, __LINE__, core_id);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

static void test_task2(void *pvParameters)
{
    int core_id;
    while (1)
    {
        core_id = xPortGetCoreID();
        printf("[%s:%d] This is a test of dongdong!, running in core %d\n", __func__, __LINE__, core_id);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/****************************************************************************************************/
/*                                       PUBLIC FUNCTIONS                                           */
/****************************************************************************************************/
void app_main(void)
{
    /* 默认创建任务方式，动态调度到可用CPU核心上 */
    //xTaskCreate(&test_task1, "test_task1", 4096, NULL, 5, NULL);
    //xTaskCreate(&test_task2, "test_task2", 4096, NULL, 5, NULL);

	/* 创建任务的同时指定任务运行的CPU核心 */
    xTaskCreatePinnedToCore(&test_task1, "test_task1", 4096, NULL, 5, NULL, CPU0);
    xTaskCreatePinnedToCore(&test_task2, "test_task2", 4096, NULL, 5, NULL, CPU1);
}
