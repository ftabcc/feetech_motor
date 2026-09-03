#include "pi_com.h"

#include <assert.h>

#include "esp_log.h"
#include "esp_err.h"

#include "tinyusb.h"
#include "tusb_cdc_acm.h"

RingbufHandle_t pi_com::rx_ringbuf = NULL;
TaskHandle_t pi_com::rx_task_handle = NULL;
const char *pi_com::TAG = "PI_COM";
static uint8_t rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE];

static void pi_com::init(void *arg)
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
}

static void pi_com::rx_task(void *arg)
{
    (void)arg; // for prevent warning: unused parameter 'arg'

    packet_t packet = {.idx = 0};
    while (1)
    {
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        while (1)
        {
            size_t item_size = 0;
            uint8_t *data = (uint8_t *)xRingbufferReceive(rx_ringbuf,&item_size,0);

            if (data == NULL)
            {break;}
            for (size_t i = 0; i < item_size; i++)
            {
                if (protocol::packet_parser(&packet,data[i]))
                    {pass}// switch case 하위함수
                // else
                //     {ESP_LOGW(TAG,"Invalid checksum");}
            }
            vRingbufferReturnItem(rx_ringbuf,(void *)data);
        }
    }
}


static void pi_com::tinyusb_cdc_rx_callback(int itf,cdcacm_event_t *event)
{
    (void)event;

    size_t rx_size = 0;
    esp_err_t ret = tinyusb_cdcacm_read(itf,rx_buf,sizeof(rx_buf),&rx_size);

    if (ret != ESP_OK)
    {ESP_LOGE(TAG, "tinyusb_cdcacm_read failed");
    return;}

    if (rx_size == 0)
    {return;}

    BaseType_t ret_rb = xRingbufferSend(rx_ringbuf,rx_buf,rx_size,0);

    if (ret_rb != pdTRUE)
    {ESP_LOGE(TAG,"RX Ring Buffer full. Data dropped: %u bytes",(unsigned)rx_size);
    return;}

    if (rx_task_handle != NULL)
    {xTaskNotifyGive(rx_task_handle);}
}
