#include <esp_system.h>

#include <stdio.h> 
#include <string.h> 
 
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" 
 
#include "tinyusb.h" 
#include "tusb_cdc_acm.h" 
 
 
void usb_init() 
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
 
 
void usb_rx_task(void *arg) 
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