#ifndef PI_COM_H
#define PI_COM_H

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"

#include "tusb_cdc_acm.h"


class pi_com
{
public:
    static void init();
private:
    static void rx_task(void *arg);
    static void tinyusb_cdc_rx_callback(int itf,cdcacm_event_t *event);


private:
    static RingbufHandle_t rx_ringbuf;
    static TaskHandle_t rx_task_handle;

    static constexpr size_t RX_RINGBUF_SIZE = 4096;
    static constexpr uint32_t RX_TASK_STACK = 4096;
    static constexpr UBaseType_t RX_TASK_PRIORITY = 10;

    static const char *TAG;
};


#endif