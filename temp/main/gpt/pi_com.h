#ifndef PI_COM_H
#define PI_COM_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "tusb_cdc_acm.h"


class pi_comm
{
public:
    static void init();
private:
    static void tinyusb_cdc_rx_callback(int itf,cdcacm_event_t *event);
    static int rxpacket(int itf);

private:
};


#endif