#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"

#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tinyusb_cdc_acm.h"
#include "sdkconfig.h"

static const char *TAG = "CDC";
#define RX_RINGBUF_SIZE   4096
#define RX_TASK_STACK     4096
#define RX_TASK_PRIORITY  10

#define JOINT_COUNT 12

static RingbufHandle_t rx_ringbuf = NULL;
static TaskHandle_t rx_task_handle = NULL;

void ESP::app_main(void)
{
    rx_ringbuf = xRingbufferCreate(RX_RINGBUF_SIZE,RINGBUF_TYPE_BYTEBUF);
    assert(rx_ringbuf != NULL);

    BaseType_t ret_task = xTaskCreate(rx_task,"rx_task",RX_TASK_STACK,NULL,RX_TASK_PRIORITY,&rx_task_handle);
    assert(ret_task == pdPASS);

    ESP_LOGI(TAG, "USB initialization");
    const tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = {
        .cdc_port = TINYUSB_CDC_ACM_0,
        .callback_rx = &tinyusb_cdc_rx_callback,
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL
    };

    ESP_ERROR_CHECK(tinyusb_cdcacm_init(&acm_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");

    joint_point_t prev_point;

    while (1)
    {vTaskDelay(pdMS_TO_TICKS(1000));}
}