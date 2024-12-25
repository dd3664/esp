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
static void create_demo_application(void)
{
	lv_obj_t *scr;
	lv_obj_t *label;
	/* Get the current screen  */
	scr = lv_disp_get_scr_act(NULL);    

	/*Create a Label on the currently active screen*/
	label =  lv_label_create(scr);

	/*Modify the Label's text*/
	lv_label_set_text(label, "Hello\nworld");

	/* Align the Label to the center
	 * NULL means align on parent (which is the screen now)
	 * 0, 0 at the end means an x, y offset after alignment*/
	lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

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

