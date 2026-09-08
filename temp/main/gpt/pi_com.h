#ifndef PI_COM_H
#define PI_COM_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "tusb_cdc_acm.h"


typedef struct
{
    size_t data_len;
    int inst;
    uint8_t data[PACKET_MAX_LEN];
    // int crc;
} pi_rx_packet_t;
// HEAD(0xFF 0xFF 0xFD) + RSRV(!0xFD) + LEN(1) + INST(1) + DATA(N) + CRC(2:L,H) = N+8(N>=0)

typedef struct
{
    size_t data_len;
    int inst;
    int err; // for emergency stop
    uint8_t data[PACKET_MAX_LEN];
    int crc;
} pi_tx_packet_t;
// HEAD(0xFF 0xFF 0xFD) + RSRV(!0xFD) + LEN(1) + INST(1) + ERR(1) + DATA(N) + CRC(2:L,H) = N+9(N>=0)

class pi_comm
{
public:
    static void init();

private:
    pi_rx_packet_t rxpacket;
    pi_tx_packet_t txpacket;
private:
    static void rx_callback(int itf,cdcacm_event_t *event);
    int rx_packet(int itf);
    uint16_t updateCRC(uint16_t start, uint8_t *addr, uint16_t size);
};



#endif