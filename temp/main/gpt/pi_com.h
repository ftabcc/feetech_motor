#pragma once

#include <cstddef>
#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "tusb_cdc_acm.h"

#include "trajectory.h"
#include "ring_buff.h"


namespace pi_protocol
{
    // Packet position
    constexpr uint16_t PKT_RESERVED     = 3;
    constexpr uint16_t PKT_LENGTH       = 4;
    constexpr uint16_t PKT_INSTRUCTION  = 5;
    constexpr uint16_t PKT_ERROR        = 6;
    constexpr uint16_t PKT_DATA         = 7;

    constexpr uint16_t RXPACKET_MAX_LEN = 100;
    constexpr uint16_t TXPACKET_MAX_LEN = 100;
    constexpr uint16_t RXPACKET_MAX_NUM = 100;
    constexpr uint16_t TXPACKET_MAX_NUM = 100;
}

// rxpacket_len = DATA(N) + 8 (FF FF FD 00 LEN INST DATA CRC_L CRC_H)
typedef struct
{
    uint16_t data_len;
    uint8_t inst;
    uint8_t data[pi_protocol::RXPACKET_MAX_LEN - 8];
} pi2esp_packet_t;

// txpacket_len = DATA(N) + 9 (FF FF FD 00 LEN INST ERR DATA CRC_L CRC_H)
typedef struct
{
    uint16_t data_len;
    uint8_t inst;
    uint8_t err;
    uint8_t data[pi_protocol::TXPACKET_MAX_LEN - 9];
} esp2pi_packet_t;

class pi_comm
{
public:
    static void init();

private:
    // INST
    enum class Inst : uint8_t
    {
        REGISTER_TRAJECTORY = 0x01,
        WRITE                = 0x02,
        STOP                 = 0x03
    };
    // COMM_RESULT
    enum class Comm_Result : uint8_t
    {
        SUCCESS        = 0,
        FAIL           = 1,
        NEED_MORE_DATA = 2,
        BUF_LEN_OVER   = 3,
        BUF_NUM_OVER   = 4,
        RX_CORRUPT     = 5,
        RX_TIMEOUT     = 6,
        CDC_ERR        = 7
    };

    QueueHandle_t rx_queue = nullptr;
    QueueHandle_t tx_queue = nullptr;

    TaskHandle_t rx_task_handle = nullptr;
    TaskHandle_t packet_process_task_handle = nullptr;
    TaskHandle_t tx_task_handle = nullptr;

    uint8_t rx_parse_buffer[RXPACKET_MAX_LEN]{};
    uint16_t rx_parse_length = 0;
    uint16_t rx_packet_len = 0;
    static constexpr uint16_t RX_DEBUG_BUFFER_SIZE = 512;
    RingBuffer rx_debug_buffer{RX_DEBUG_BUFFER_SIZE};




    static void rx_callback(int itf, cdcacm_event_t *event);
    static void rx_task(void *arg);
    Comm_Result rx_packet(pi2esp_packet_t &rxpacket);


    static void packet_process_task(void *arg);

    static void tx_task(void *arg);
    Comm_Result tx_packet(const esp2pi_packet_t &txpacket);


    uint16_t updateCRC(uint16_t start, uint8_t *addr, uint16_t size);
    Comm_Result stuffing(uint8_t *data, uint16_t *len);
    Comm_Result unstuffing(uint8_t *data, uint16_t *len);


    // INST
    Trajectory trajectory;
};

extern pi_comm pi_comm_instance;