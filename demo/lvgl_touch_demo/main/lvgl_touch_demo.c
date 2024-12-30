/****************************************************************************************************/
/*                                           INCLUDE                                                */
/****************************************************************************************************/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lvgl.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "lvgl_helpers.h"
#include "esp_timer.h"
/****************************************************************************************************/
/*                                           DEFINES                                                */
/****************************************************************************************************/
#define LV_TICK_PERIOD_MS 1

/****************************************************************************************************/
/*                                           VARIABLES                                              */
/****************************************************************************************************/
/* Creates a semaphore to handle concurrent call to lvgl stuff
 * If you wish to call *any* lvgl function from other threads/tasks
 * you should lock on the very same semaphore! */
SemaphoreHandle_t xGuiSemaphore;

/****************************************************************************************************/
/*                                       STATIC FUNCTIONS                                           */
/****************************************************************************************************/
static void touch_init(void)
{
	static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_driver_read; // 使用 lvgl_esp32_drivers 的触摸读取函数
    lv_indev_drv_register(&indev_drv);
	return;
}

static void btn_event_cb(lv_event_t * e)
{
	
    static uint8_t cnt = 0;
    lv_event_code_t code = 0;
    lv_obj_t * btn = NULL;
	lv_obj_t *label = NULL;

	code = lv_event_get_code(e);
	btn = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
        cnt++;
        /*Get the first child of the button which is the label and change its text*/
        label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Button: %d", cnt);
    }
	return;
}

static void create_demo_application(void)
{
	lv_obj_t * btn = NULL;
	lv_obj_t * label = NULL;

	btn = lv_btn_create(lv_scr_act());     /*Add a button the current screen*/
	lv_obj_set_pos(btn, 10, 10);                            /*Set its position*/
	lv_obj_set_size(btn, 120, 50);                          /*Set its size*/
	lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);           /*Assign a callback to the button*/

	label = lv_label_create(btn);          /*Add a label to the button*/
	lv_label_set_text(label, "Button");                     /*Set the labels text*/
	lv_obj_center(label);	
	return;
}

static void lv_tick_task(void *arg) {
	lv_tick_inc(LV_TICK_PERIOD_MS);
	
	return;
}

static void gui_task(void *pvParameter)
{
	lv_color_t *buf1 = NULL;
	lv_color_t* buf2 = NULL;
	static lv_disp_draw_buf_t disp_buf;
	uint32_t size_in_px = DISP_BUF_SIZE;
	lv_disp_drv_t disp_drv;
	const esp_timer_create_args_t periodic_timer_args = {
		.callback = &lv_tick_task,
		.name = "periodic_gui"
	};
	esp_timer_handle_t periodic_timer;

	xGuiSemaphore = xSemaphoreCreateMutex();
	lv_init();
	/* Initialize SPI or I2C bus used by the drivers */
	lvgl_driver_init();
	buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
	assert(buf1 != NULL);
	/* Use double buffered when not working with monochrome displays */
	buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
	assert(buf2 != NULL);
	/* Initialize the working buffer depending on the selected display. */
	lv_disp_draw_buf_init(&disp_buf, buf1, buf2, size_in_px);
	lv_disp_drv_init(&disp_drv);
	disp_drv.hor_res = LV_HOR_RES_MAX;
	disp_drv.ver_res = LV_VER_RES_MAX;
	disp_drv.flush_cb = disp_driver_flush;
	disp_drv.draw_buf = &disp_buf;
	lv_disp_drv_register(&disp_drv);
	/* Create and start a periodic timer interrupt to call lv_tick_inc */
	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));
	
	touch_init();

	/* Create the demo application */
	create_demo_application();
	while (1)
	{
		/* Try to take the semaphore, call lvgl related function on success */
		if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
			lv_task_handler();
			xSemaphoreGive(xGuiSemaphore);
		}
		/* Delay 10ms */
		vTaskDelay(pdMS_TO_TICKS(10));

	}
	return;
}

/****************************************************************************************************/
/*                                       PUBLIC FUNCTIONS                                           */
/****************************************************************************************************/
void app_main(void)
{
	xTaskCreate(gui_task, "gui", 4096*2, NULL, 1, NULL);
	return;
}

