
#include "esp_log.h"
#include "esp_err.h"
#include "stdbool.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_freertos_hooks.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "lvgl/lvgl.h"
#include "ili9486_config.h"
#include "ili9486.h"

static SemaphoreHandle_t guiSemaphore;

static void lv_tick_increase_task(void *args)
{
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void create_demo_screen(lv_obj_t *screen)
{
    ESP_LOGI(__FILE__, "Creating a styled screen!!");
    static lv_style_t screen_style;
    memset(&screen_style, 0, sizeof(lv_style_t));
    lv_style_init(&screen_style);
    lv_style_set_bg_color(&screen_style, lv_palette_main(LV_PALETTE_GREEN));
    lv_obj_add_style(screen, &screen_style, 0);

}

static void lvgl_gui_task(void *params)
{
    ili9486_init();
    guiSemaphore = xSemaphoreCreateMutex();
    static lv_disp_drv_t display_driver;
    static lv_disp_draw_buf_t display_draw_buffer;

    lv_init();

    lv_color16_t *buffer_1 = (lv_color16_t *)heap_caps_malloc(ILI9486_DISPLAY_BUFFER * sizeof(lv_color16_t), MALLOC_CAP_DMA);
    assert(buffer_1);
    lv_color16_t *buffer_2 = (lv_color16_t *)heap_caps_malloc(ILI9486_DISPLAY_BUFFER * sizeof(lv_color16_t), MALLOC_CAP_DMA);
    assert(buffer_2);

    ESP_LOGI(__FILE__, "Buffer 1 is %s", ((NULL == buffer_1) ? "NULL" : "NOT NULL"));
    ESP_LOGI(__FILE__, "Buffer 2 is %s", ((NULL == buffer_2) ? "NULL" : "NOT NULL"));

    uint32_t size_in_px = ILI9486_DISPLAY_BUFFER;
    lv_disp_draw_buf_init(&display_draw_buffer, buffer_1, buffer_2, size_in_px);

    lv_disp_drv_init(&display_driver);
    display_driver.hor_res = ILI9486_HOR_RES;
    display_driver.ver_res = ILI9486_VER_RES;
    display_driver.flush_cb = ili9486_flush;
    display_driver.draw_buf = &display_draw_buffer;
    display_driver.user_data = NULL;

    lv_disp_t *display = lv_disp_drv_register(&display_driver);

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_increase_task,
        .name = "periodic gui",
    };

    esp_timer_handle_t periodic_timer_handler;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer_handler));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer_handler, LV_TICK_PERIOD_MS * 1000));

    lv_obj_t *current_screen = lv_disp_get_scr_act(display);

    create_demo_screen(current_screen);

    while(1)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        ESP_LOGI(__FILE__, "semaphore is %s", ((NULL == guiSemaphore) ? "NULL" : "NOT NULL"));
        if(pdTRUE == xSemaphoreTake(guiSemaphore, portMAX_DELAY))
        {
            lv_task_handler();
            xSemaphoreGive(guiSemaphore);
        }
    }

}

void app_main()
{
    ESP_LOGI(__FILE__, "HERE!!");
    xTaskCreate(lvgl_gui_task, "lvgl_gui_task", 4096 * 2, NULL, 0, NULL);
}
