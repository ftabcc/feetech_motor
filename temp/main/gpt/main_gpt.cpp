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

#define MY_ERR 20
#define JOINT_COUNT 12

#define INST_REGISTER_TRAJECTORY  0x01
#define INST_STOP                 0x02
#define INST_CLEAR_TRAJECTORY     0x03

static RingbufHandle_t rx_ringbuf = NULL;
static TaskHandle_t rx_task_handle = NULL;

static uint8_t rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE];

#define PACKET_MAX_LEN 100

typedef struct
{
    uint8_t buffer[PACKET_MAX_LEN + 3];
    size_t len; // packet_total_LEN = HEAD(2) + LEN(1) + INST(1) + DATA(N) + CHECKSUM(1) = N+5 (N>=0)
    size_t idx;

} packet_t;

typedef struct
{
    uint8_t data[2+12*7]; // packet_data = TIME(2) + 12*[ACC(1) + POS(2) + MAX_TIME(2) + VEL(2)]
    uint32_t time_ms;

    float q[12];
    float v[12];
    float a[12];
} joint_point_t;

typedef struct
{
    uint32_t time_ms;
    float q[JOINT_COUNT];
    float v[JOINT_COUNT];
    float a[JOINT_COUNT];
    //uint16_t max_time[12];
} joint_point_t;

bool register_joint_trajectory(const packet_t *packet,joint_point_t *point)
{
    const size_t data_len = 2 + JOINT_COUNT * (1 + 2 + 2 + 2); // packet_data = TIME(2) + 12*[ACC(1) + POS(2) + MAX_TIME(2) + VEL(2)]
    if (packet->len < 4 + data_len + 1)
        return false;
    if (packet == NULL || point == NULL)
        return false;    

    size_t idx = 4;

    // TIME(2)
    uint16_t time = ((uint16_t)packet->buffer[idx] << 8)|(uint16_t)packet->buffer[idx + 1];
    point->time_ms = time;
    idx += 2;

    for (int i = 0; i < JOINT_COUNT; i++)
    {
        // ACC(1)
        uint8_t acc = packet->buffer[idx];
        idx += 1;
        // POS(2)
        uint16_t pos = ((uint16_t)packet->buffer[idx] << 8)|(uint16_t)packet->buffer[idx + 1];
        idx += 2;
        // MAX_TIME(2)
        uint16_t max_time = ((uint16_t)packet->buffer[idx] << 8)|(uint16_t)packet->buffer[idx + 1];
        idx += 2;
        // VEL(2)
        uint16_t vel = ((uint16_t)packet->buffer[idx] << 8)|(uint16_t)packet->buffer[idx + 1];
        idx += 2;
        // joint_point_t에 저장
        point->a[i] = (float)acc;
        point->q[i] = (float)pos;
        // point->max_time[i] = max_time;
        point->v[i] = (float)vel;
    }
    return true;
}

static int ESP::packet_parser(packet_t *packet,uint8_t byte)
{
    /*패킷을 읽던중 ff,ff가 들어오면 새 패킷으로 시작하지 않고
    기존에 읽던 패킷을 len을 참고해서 다 읽고,
    check_sum확인했을때 맞다면 중간에 읽은 ff,ff를
    기존 패킷의 데이터의 일부라고 보도록 하는게 합리적*/
    // 체크섬 실패시 버렸던 버퍼에서 헤더 유무파악하기

    /* 1. 첫 번째 FF 탐색 */
    if (packet->idx == 0)
    {
        if (byte == 0xFF)
        {
            packet->buffer[0] = byte;
            packet->idx = 1;
        }
        return false;
    }

    /* 2. 두 번째 FF 확인 */
    if (packet->idx == 1)
    {
        if (byte == 0xFF)
        {
            packet->buffer[1] = byte;
            packet->idx = 2;}
        else
        {packet->idx = 0;}
        return false;
    }

    if (packet->idx >= sizeof(packet->buffer))
    {
        packet->idx = 0;
        packet->len = 0;
        return false;
    }

    packet->buffer[packet->idx] = byte;
    packet->idx++;

    /* 4. LEN 확인 */
    if (packet->idx == 3)
    {
        packet->len = packet->buffer[2];

        if (packet->len < 5)
        {
            packet->idx = 0;
            packet->len = 0;
            return false;
        }

        if (packet->len > sizeof(packet->buffer))
        {
            packet->idx = 0;
            packet->len = 0;
            return false;
        }
    }

    /* 5. 패킷 완성 확인 */
    if (packet->idx >= 5 && packet->idx == packet->len)
    {
        uint16_t sum = 0;
        for (size_t i = 2; i < packet->idx - 1; i++)
        {sum += packet->buffer[i];} // without header(0xff,0xff)
        uint8_t checksum = (uint8_t)(sum & 0xFF);

        if (checksum == packet->buffer[packet->len - 1])
        {   packet->idx = 0;
            packet->len = 0;
            uint8_t inst = packet->buffer[3];

            switch (inst)
            {
                case INST_REGISTER_TRAJECTORY:
                    register_joint_trajectory(packet);
                    break;

                case INST_STOP:
                    stop_motor();
                    break;

                case INST_CLEAR_TRAJECTORY:
                    clear_trajectory();
                    break;

                default:
                    // 알 수 없는 INST
                    break;
            }
            return true;}
    }
    return false;
}

static void ESP::tinyusb_cdc_rx_callback(int itf,cdcacm_event_t *event)
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

static void ESP::rx_task(void *arg)
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
                if (packet_parser(&packet,data[i]))
                    {pass}// switch case 하위함수
                // else
                //     {ESP_LOGW(TAG,"Invalid checksum");}
            }
            vRingbufferReturnItem(rx_ringbuf,(void *)data);
        }
    }
}

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

    while (1)
    {vTaskDelay(pdMS_TO_TICKS(1000));}
}