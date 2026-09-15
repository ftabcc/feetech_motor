#ifndef PI_COM_H
#define PI_COM_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tusb_cdc_acm.h"

#define INST_REGISTER_TRAJECTORY  0x01
#define INST_STOP                 0x02
#define INST_CLEAR_TRAJECTORY     0x03

#define COMM_SUCCESS        0
#define COMM_FAIL           1
#define COMM_RX_CORRUPT     2
#define COMM_BUF_OVER       3
#define COMM_RX_TIMEOUT     4
#define COMM_CDC_ERR        5

RXPACKET_MAX_LEN = 

PKT_RESERVED = 
PKT_LENGTH = 
PKT_INSTRUCTION = 

#define PACKET_MAX_LEN 100
#define PACKET_BUFFER_SIZE 8

typedef struct
{
    size_t len;
    int inst;
    uint8_t data[PACKET_MAX_LEN];
    // int crc;
} pi2esp_packet_t;
// HEAD(0xFF 0xFF 0xFD) + RSRV(!0xFD) + LEN(1) + INST(1) + DATA(N) + CRC(2:L,H) = N+8(N>=0)

typedef struct
{
    pi2esp_packet_t packets[PACKET_BUFFER_SIZE];
    size_t write_idx;
    size_t read_idx;
    size_t count;
} pi2esp_packet_buffer_t;

typedef struct
{
    size_t len;
    int inst;
    int err; // for emergency stop
    uint8_t data[PACKET_MAX_LEN];
    int crc;
} esp2pi_packet_t;
// HEAD(0xFF 0xFF 0xFD) + RSRV(!0xFD) + LEN(1) + INST(1) + ERR(1) + DATA(N) + CRC(2:L,H) = N+9(N>=0)


class pi_comm
{
public:
    static void init();

private:
    pi2esp_packet_buffer_t rxpacket_buffer;
    esp2pi_packet_t txpacket;

private:
    static void rx_callback(int itf,cdcacm_event_t *event);
    int rx_packet(int itf);
    uint16_t updateCRC(uint16_t start, uint8_t *addr, uint16_t size);

    Trajectory trajectory;
};



#endif