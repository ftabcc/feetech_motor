#include <esp_system.h>

#include <stdio.h> 
#include <string.h> 
 
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" 
 
#include "tinyusb.h" 
#include "tusb_cdc_acm.h" 
 
 
void ESP::native_usb_init() 
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


//test기반에서 큐를 링버퍼로 교환하고 노티파이 vs xQueueReceive(큐리시브 말고 링버퍼리시브함수있나?)

void ESP::native_usb_rx_callback(int itf,cdcacm_event_t *event)
    {
        ESPMain *self = static_cast<ESPMain*>(event->user_arg);
        xTaskNotifyGive(self->native_usb_rx_task_handle);
    }

void ESP::native_usb_rx_task(void *arg) 
{ 
    uint8_t buffer[256]; 
    while (true) 
    { 
        
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        
        uint8_t bTemp;
        size_t rx_size; //how much read 
        
        u8 bBuf[] = {0, 0};
        u8 cnt = 0;
        esp_err_t result;
        find_head = false
        while(true){
            result = tinyusb_cdcacm_read(TINYUSB_CDC_ACM_0,&bTemp,1,&rx_size); // read 1byte
            if(result == ESP_OK){
            bBuf[1] = bBuf[0];
            bBuf[0] = bTemp;
            if(bBuf[0]==0xff && bBuf[1]==0xff){
                find_head = true;
                break;}
            cnt++;
            if(cnt>10){
                find_head = false; 
                break;}}}

        uint8_t packet_len;
        result = tinyusb_cdcacm_read(TINYUSB_CDC_ACM_0,&packet_len,1,&rx_size);
        uint8_t data_len = packet_len - 1;
        if(result == ESP_OK){
            result = tinyusb_cdcacm_read(TINYUSB_CDC_ACM_0,buffer,packet_len,&rx_size);
            if(result == ESP_OK && rx_size == packet_len){
                uint8_t inst = buffer[0]
                uint8_t check_sum = packet_len + inst;
                uint8_t* data = new uint8_t[data_len]
                for(size_t i=0; i<data_len; i++){
                    check_sum += buffer[i+1];
                    data[i] = buffer[i+1]}
                    
                check_sum = ~check_sum;
                if(check_sum!=buffer[-1]){
                    Error = ERR_check_sum;}
                if()
            }
        }


        if (result == ESP_OK && ) 
        { 
            
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
    xTaskCreate(usb_rx_task, "usb_rx_task", 4096, nullptr, 5, nullptr ); 
}

void ESP::rx_pi_packet()
{

}

void ESP::control_task(void *arg)
{
    TickType_t last_wake = xTaskGetTickCount();

    while (true)
    {
        // ===== 20ms마다 실행할 코드 =====
        control_motor();

        // 다음 20ms 주기까지 대기
        vTaskDelayUntil(
            &last_wake,
            pdMS_TO_TICKS(20)
        );
    }
}

void ESP::quintic_heremit()
{

}