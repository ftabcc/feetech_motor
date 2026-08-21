#include <esp_system.h>

#include <stdio.h> 
#include <string.h> 
 
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" 
 
#include "tinyusb.h" 
#include "tusb_cdc_acm.h" 
 
 
void esp_serial::usb_init() 
{ 
    tinyusb_config_t tusb_cfg = {}; 
    ESP_ERROR_CHECK( tinyusb_driver_install(&tusb_cfg) ); 
    tinyusb_config_cdcacm_tacm_config_t cdc_cfg = {}; 
 
    cdc_cfg.usb_dev = TINYUSB_USBDEV_0; 
    cdc_cfg.cdc_port = TINYUSB_CDC_ACM_0; 
    cdc_cfg.rx_unread_buf_sz = 256; 
    cdc_cfg.callback_rx = nullptr; // to rx패킷분석기
 
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&cdc_cfg)); 
} 
 
 
void esp_serial::usb_rx_task(void *arg) 
{ 
    uint8_t buffer[256]; 
    while (true) 
    { 
        size_t rx_size = 0; //how much read 
        esp_err_t result = tinyusb_cdcacm_read(TINYUSB_CDC_ACM_0,buffer,sizeof(buffer),&rx_size); 
        if (result == ESP_OK && rx_size > 0) 
        { 
            printf("RX: %.*s\n", (int)rx_size, buffer ); 
            const char response[] = "ESP32 received!\r\n"; 
            tinyusb_cdcacm_write_queue( TINYUSB_CDC_ACM_0, (const uint8_t *)response, strlen(response) ); 
            tinyusb_cdcacm_write_flush( TINYUSB_CDC_ACM_0 ); 
        } 
 
        vTaskDelay(pdMS_TO_TICKS(1)); 
    } 
} 

extern "C" void app_main() 
{ 
    usb_init(); 
    xTaskCreate( usb_rx_task, "usb_rx_task", 4096, nullptr, 5, nullptr ); 
}




#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tinyusb.h"
#include "tusb_cdc_acm.h"


class ESPMain
{
private:
    TaskHandle_t rx_task_handle = nullptr;

public:

    // -------------------------
    // USB 초기화
    // -------------------------
    void usb_init()
    {
        tinyusb_config_t tusb_cfg = {};

        ESP_ERROR_CHECK(
            tinyusb_driver_install(&tusb_cfg)
        );

        tinyusb_config_cdcacm_tacm_config_t cdc_cfg = {};

        cdc_cfg.usb_dev = TINYUSB_USBDEV_0;
        cdc_cfg.cdc_port = TINYUSB_CDC_ACM_0;

        cdc_cfg.rx_unread_buf_sz = 256;

        // RX 데이터가 들어오면 호출될 callback
        cdc_cfg.callback_rx = usb_rx_callback;

        ESP_ERROR_CHECK(
            tusb_cdc_acm_init(&cdc_cfg)
        );
    }


    // -------------------------
    // USB RX callback
    // -------------------------
    static void usb_rx_callback(
        int itf,
        cdcacm_event_t *event
    )
    {
        // 실제 데이터 처리는 하지 않는다.
        // RX Task에게 "데이터가 왔다"고 알림.

        ESPMain *self =
            static_cast<ESPMain*>(event->user_arg);

        xTaskNotifyGive(self->rx_task_handle);
    }


    // -------------------------
    // RX Task
    // -------------------------
    static void rx_task_entry(void *arg)
    {
        ESPMain *self =
            static_cast<ESPMain*>(arg);

        while (true)
        {
            // 데이터가 올 때까지 Block
            ulTaskNotifyTake(pdTRUE,portMAX_DELAY);

            // -------------------------
            // 실제 USB 데이터 읽기
            // -------------------------

            uint8_t buffer[256];
            size_t rx_size = 0;

            esp_err_t result =
                tinyusb_cdcacm_read(
                    TINYUSB_CDC_ACM_0,
                    buffer,
                    sizeof(buffer),
                    &rx_size
                );

            if (result == ESP_OK && rx_size > 0)
            {
                printf(
                    "RX: %.*s\n",
                    (int)rx_size,
                    buffer
                );
            }
        }
    }


    // -------------------------
    // 시작
    // -------------------------
    void start()
    {
        usb_init();
        xTaskCreate(rx_task_entry,"rx_task",4096,this,5,&rx_task_handle);
    }
};

