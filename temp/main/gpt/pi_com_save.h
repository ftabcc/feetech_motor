#ifndef PI_COM_H
#define PI_COM_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tusb_cdc_acm.h"

#include "trajectory.h"
#include "ring_buff.h"

// esp<->pi protocol
typedef struct
{
    size_t data_len;
    int inst;
    uint8_t data[RXPACKET_MAX_LEN-8];
    // int crc;
} pi2esp_packet_t;
// HEAD(0xFF 0xFF 0xFD) + RSRV(!0xFD) + LEN(1) + INST(1) + DATA(N) + CRC(2:L,H) = N+8(N>=0)

typedef struct
{
    pi2esp_packet_t packets[RXPACKET_MAX_NUM];
    size_t write_idx;
    size_t read_idx;
    size_t count;
} pi2esp_packets_t;

typedef struct
{
    size_t data_len;
    int inst = 55; //reply inst
    int err;
    uint8_t data[TXPACKET_MAX_LEN-9];
    int crc;
} esp2pi_packet_t;
// HEAD(0xFF 0xFF 0xFD) + RSRV(!0xFD) + LEN(1) + INST(1) + ERR(1) + DATA(N) + CRC(2:L,H) = N+9(N>=0)

typedef struct
{
    esp2pi_packet_t packets[TXPACKET_MAX_NUM];
    size_t write_idx;
    size_t read_idx;
    size_t count;
} esp2pi_packets_t;

class pi_comm
{
public:
    static void init();

private:
    
    

    // HEAD(0xFF 0xFF 0xFD) + RSRV(!0xFD) + LEN(1) + INST(1) + DATA(N) + CRC(2:L,H) = N+8(N>=0)
    #define PKT_RESERVED = 3
    #define PKT_LENGTH = 4
    #define PKT_INSTRUCTION = 5

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
        BUF_LEN_OVER = 2,
        BUF_NUM_OVER = 3,
        RX_CORRUPT = 4,
        RX_TIMEOUT = 5,
        CDC_ERR = 6
    };

    enum class Inst_Result
    {

    }


private:

    static void rx_callback(int itf,cdcacm_event_t *event);
    static void rx_task(void *arg);
    Comm_Result rx_packet();
    
    static void packet_process_task(void *arg);

    Comm_Result tx_packet(int itf, const pi_tx_packet_t &packet);
    static void tx_task(void *arg);

    uint16_t updateCRC(uint16_t start, uint8_t *addr, uint16_t size);    
    int pi_comm::stuffing(uint8_t *data, int *len);
    int pi_comm::unstuffing(uint8_t *data, int *len);

    
    pi2esp_packet_t rxpacket; // for temp rxpacket before send queue
    esp2pi_packet_t txpacket; // for temp rxpacket before send queue

    QueueHandle_t rx_queue;
    QueueHandle_t tx_queue;
    // packet size
    static constexpr uint16_t RXPACKET_MAX_LEN = 100; // rxpacket_len = data(n) + 8
    static constexpr uint16_t TXPACKET_MAX_LEN = 100; // txpacket_len = data(n) + 9
    // packet buffer size
    static constexpr uint16_t RXPACKET_MAX_NUM = 100;
    static constexpr uint16_t TXPACKET_MAX_NUM = 100;
    
    RingBuffer rx_parse_buffer;
    RingBuffer rx_debug_buffer;
    Trajectory trajectory;

};

extern pi_comm pi_comm_instance;
#endif