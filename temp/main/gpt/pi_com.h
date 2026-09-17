#ifndef PI_COM_H
#define PI_COM_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tusb_cdc_acm.h"

// esp<->pi protocol



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
    static constexpr uint16_t TXPACKET_MAX_LEN = 100;
    static constexpr uint16_t RXPACKET_MAX_LEN = 100;
    
    #define PACKET_BUFFER_SIZE 8

    #define PKT_RESERVED = 
    #define PKT_LENGTH = 
    #define PKT_INSTRUCTION = 

    enum class Inst : uint8_t
    {
        REGISTER_TRAJECTORY = 0x01,
        WRITE = 0x02,
        STOP = 0x03
    };

    enum class Comm_Result
    {
        SUCCESS = 0,
        FAIL = 1,
        BUF_OVER = 2,
        RX_CORRUPT = 3,
        RX_TIMEOUT = 4,
        CDC_ERR = 5
    };


private:
    pi2esp_packet_buffer_t rxpacket_buffer;
    esp2pi_packet_t txpacket;

    static void rx_callback(int itf,cdcacm_event_t *event);
    static void packet_process_task(void *arg);

    int rx_packet(int itf);
    uint16_t updateCRC(uint16_t start, uint8_t *addr, uint16_t size);

    Trajectory trajectory;
};

extern pi_comm pi_comm_instance;
#endif