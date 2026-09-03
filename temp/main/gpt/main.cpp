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

    while (1)
    {vTaskDelay(pdMS_TO_TICKS(1000));}
}