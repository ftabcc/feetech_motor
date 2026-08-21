#include <esp_system.h>

extern "C" void app_main() 
{ 
    usb_init(); 
    xTaskCreate( usb_rx_task, "usb_rx_task", 4096, nullptr, 5, nullptr ); 
}