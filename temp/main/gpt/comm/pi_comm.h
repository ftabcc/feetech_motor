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
    uint8_t HEADER_LEN = 4;
    // Packet Idx
    constexpr uint16_t RX_RESERVED_IDX     = 3;
    constexpr uint16_t RX_LENGTH_IDX       = 4;
    constexpr uint16_t RX_ID_IDX           = 5;
    constexpr uint16_t RX_INSTRUCTION_IDX  = 6;
    constexpr uint16_t RX_DATA_IDX         = 7;
    constexpr uint16_t TX_RESERVED_IDX     = 3;
    constexpr uint16_t TX_LENGTH_IDX       = 4;
    constexpr uint16_t TX_ID_IDX           = 5;
    constexpr uint16_t TX_INSTRUCTION_IDX  = 6;
    constexpr uint16_t TX_ERROR_IDX        = 7;
    constexpr uint16_t TX_DATA_IDX         = 8;

    // packet length range
    constexpr uint8_t RX_PACKET_MAX_LEN = 255; // LEN field is 1 byte
    constexpr uint8_t RX_PACKET_MIN_LEN = 11;
    constexpr uint8_t TX_PACKET_MAX_LEN = 255;
    constexpr uint8_t TX_PACKET_MIN_LEN = 12;
    constexpr uint8_t RX_PACKET_MAX_NUM = 100;
    constexpr uint8_t TX_PACKET_MAX_NUM = 100;

    // rx_timeout : 더 이상 확인할 바이트가 없는 상태유지 허용임계점 
    // '''너무 짧으면 잠깐 끊긴 정상패킷을 버리게 될수도있으니 적당하게 잡아야함'''
    constexpr uint8_t RX_TIMEOUT       = 0.1;
    constexpr uint8_t RX_TIMEOUT_MS    = RX_TIMEOUT * 1000;
    constexpr uint16_t RX_TIMEOUT_US   = RX_TIMEOUT_MS * 1000;

    
    // rxpacket_len = DATA(N) + 9 (FF FF FD 00 LEN ID INST DATA CRC_L CRC_H)
    struct rxpacket_t {
    uint8_t data_len;
    uint16_t id;
    uint8_t inst;
    uint8_t data[pi_protocol::RXPACKET_MAX_LEN - 8];
    };

    // txpacket_len = DATA(N) + 10 (FF FF FD 00 LEN ID INST ERR DATA CRC_L CRC_H)
    struct txpacket_t{
    uint8_t data_len;
    uint16_t id;
    uint8_t inst;
    uint8_t err;
    uint8_t data[pi_protocol::TXPACKET_MAX_LEN - 9];
    };

    // COMM_status
    enum class rx_result : uint8_t{
        SUCCESS        = 0,
        WAITING        = 1,  //INCOMPLETE, need more data, Pending, wait
        FAILURE        = 2,
        NOTHING        = 3
    };
    enum class rx_error : uint8_t{
        CRC_ERR        = 1 << 0,  // 0000 0001
        BUFFER_FULL    = 1 << 1,  // 0000 0010
        RX_TIMEOUT     = 1 << 2,  // 0000 0100
        CDC_ERR        = 1 << 3,  // 0000 1000
        // ???         = 1 << 4,  // 0001 0000
        // ???         = 1 << 5,  // 0010 0000
        // ???         = 1 << 6,  // 0100 0000
        // ???         = 1 << 7,  // 1000 0000
        
    };

    // INST
    enum class Inst : uint8_t
    {
        REGISTER_TRAJECTORY  = 0x01,
        WRITE                = 0x02,
        STOP                 = 0x03
    };
}

class pi_comm
{
public:
    static void init();
private:
    
    QueueHandle_t rx_queue = nullptr;
    QueueHandle_t tx_queue = nullptr;
    TaskHandle_t rx_task_handle = nullptr;
    // TaskHandle_t packet_process_task_handle = nullptr;
    // TaskHandle_t tx_task_handle = nullptr;

    uint8_t rx_buffer[pi_protocol::RXPACKET_MAX_LEN*N]{};
    int64_t rx_wait_start_time_us = 0;
    size_t idx = 0; // 0~ max_len*n
    size_t read_available = 0;
    bool found = false;
    bool confirm = false;
    uint8_t rx_errors = 0;
    
    static void rx_callback(int itf, cdcacm_event_t *event);

    static void rx_task(void *arg);
    pi_protocol::rx_result rx_packet(pi_protocol::rxpacket_t &rxpacket);
    static void tx_task(void *arg);
    pi_protocol::Comm_Result tx_packet(pi_protocol::txpacket_t &txpacket);

    static void packet_process_task(void *arg);
    
    uint16_t updateCRC(uint16_t start, uint8_t *addr, uint16_t size);
    pi_protocol::Comm_Result stuffing(uint8_t *data, uint16_t *len);
    pi_protocol::Comm_Result unstuffing(uint8_t *data, uint16_t *len);

    // INST
    Trajectory trajectory;
};

extern pi_comm pi_comm_instance;