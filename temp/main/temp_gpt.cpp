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

static RingbufHandle_t rx_ringbuf = NULL;
static TaskHandle_t rx_task_handle = NULL;

static uint8_t rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE];

typedef struct
{
    uint8_t *data;
    size_t len;
} packet_t;

typedef struct
{
    uint8_t buffer[256];
    size_t len;

} packet_parser_t;


static bool packet_parser_push(
    packet_parser_t *parser,
    uint8_t byte,
    packet_t *packet)
{
    (void)parser;
    (void)byte;
    (void)packet;

    /*
     * TODO:
     *
     * 실제 프로토콜에 맞춰 구현.
     *
     * 예:
     * 1. header 탐색
     * 2. packet length 확인
     * 3. 필요한 byte 수집
     * 4. checksum 수집
     * 5. packet 완성 여부 판단
     */

    return false;
}


static bool packet_checksum_valid(const packet_t *packet)
{
    (void)packet;

    /*
     * TODO:
     * 실제 프로토콜의 checksum 계산법 구현.
     */

    return false;
}


static void dispatch_packet(const packet_t *packet)
{
    (void)packet;

    /*
     * TODO:
     *
     * 실제 프로토콜에 맞춰 INST를 읽고
     * 하위 함수를 호출한다.
     *
     * 예:
     *
     * switch (inst)
     * {
     *     case ...:
     *         ...
     *         break;
     *
     *     case ...:
     *         ...
     *         break;
     * }
     */
}

static void tinyusb_cdc_rx_callback(int itf,cdcacm_event_t *event)
{
    (void)event;

    size_t rx_size = 0;
    esp_err_t ret = tinyusb_cdcacm_read(itf,rx_buf,sizeof(rx_buf),&rx_size);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "tinyusb_cdcacm_read failed");
        return;}

    if (rx_size == 0)
    {return;}

    BaseType_t ret_rb = xRingbufferSend(rx_ringbuf,rx_buf,rx_size,0);

    if (ret_rb != pdTRUE)
    {
        ESP_LOGE(
            TAG,
            "RX Ring Buffer full. Data dropped: %u bytes",
            (unsigned)rx_size
        );

        return;
    }

    if (rx_task_handle != NULL)
    {
        xTaskNotifyGive(rx_task_handle);
    }
}


static void rx_task(void *arg)
{
    (void)arg; // for prevent warning: unused parameter 'arg'

    packet_parser_t parser = {.len = 0};
    packet_t packet;
    while (1)
    {
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        while (1)
        {
            size_t item_size = 0;
            uint8_t *data = (uint8_t *)xRingbufferReceive(rx_ringbuf,&item_size,0);

            if (data == NULL)
            {
                break;
            }
            for (size_t i = 0; i < item_size; i++)
            {
                if (packet_parser_push(&parser,data[i],&packet))
                {
                    if (packet_checksum_valid(&packet))
                    {dispatch_packet(&packet);}
                    else
                    {ESP_LOGW(TAG,"Invalid checksum");}
                }
            }

            /*
             * Ring Buffer에서 받은 데이터를
             * 사용 완료했음을 알려준다.
             */
            vRingbufferReturnItem(rx_ringbuf,(void *)data);
        }
    }
}


void app_main(void)
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

    while (1)
    {vTaskDelay(pdMS_TO_TICKS(1000));}
}