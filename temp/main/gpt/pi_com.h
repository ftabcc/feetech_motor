#ifndef PI_COM_H
#define PI_COM_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "tusb_cdc_acm.h"


typedef struct
{
    uint8_t data[PACKET_MAX_LEN];
    size_t packet_len;// packet_total_LEN = HEAD(2) + LEN(1) + INST(1) + DATA(N) + CHECKSUM(1) = N+5 (N>=0)
    size_t data_len;
    int inst;
    int u8 u8Error;
    int crc;
} packet_t;


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