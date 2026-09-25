#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "esp_err.h"
#include "esp_timer.h"

#include "tinyusb_cdc_acm.h"

constexpr uart_port_t MOTOR_UART = UART_NUM_1;
constexpr int MOTOR_TX_PIN  = 18;
constexpr int MOTOR_RX_PIN  = 19;

constexpr int MOTOR_BAUDRATE = 1000000;
constexpr uint8_t MOTOR_COUNT = 12;

constexpr int PI_CDC_ITF = TINYUSB_CDC_ACM_0;
// ============================================================
// ST3215-HS SYNC READ packet
//
// Start address : 41 (0x29)
// Read length   : 19 (0x13)
// addr
//   41      : Acceleration
//   42~43   : Goal Position
//   44~45   : Running Time
//   46~47   : Goal Speed
//   48~49   : Torque Limit
//   ...
//   56~57   : Present Position
//   58~59   : Present Speed
//
// IDs = 0~11
//
// Packet:
// FF FF FE 10 82 29 13
// 00 01 02 03 04 05 06 07 08 09 0A 0B
// F5
// ============================================================

constexpr uint8_t SYNC_READ_PACKET[] =
{
    0xFF, 0xFF, //header
    0xFE, //brodcast id
    0x10, //len = 16
    0x82, //inst = syncread = 
    0x29, //addr = 41
    0x13, //read_len?

    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
    0xF5
};
constexpr size_t SYNC_READ_PACKET_LEN = sizeof(SYNC_READ_PACKET);


// 1 status packet:
// Header(2) + ID(1) + Length(1) + Error(1)
// + Data(19) + CRC(1) = 25 bytes
constexpr size_t STATUS_PACKET_LEN = 25;
constexpr size_t EXPECTED_RX_LEN = MOTOR_COUNT * STATUS_PACKET_LEN;
TaskHandle_t benchmark_task_handle = nullptr;

// Send measured time to Raspberry Pi// 4 bytes, little endian
void send_time_to_pi(uint32_t time_us)
{
    uint8_t packet[4];

    packet[0] = static_cast<uint8_t>(time_us);
    packet[1] = static_cast<uint8_t>(time_us >> 8);
    packet[2] = static_cast<uint8_t>(time_us >> 16);
    packet[3] = static_cast<uint8_t>(time_us >> 24);

    tinyusb_cdcacm_write_queue(PI_CDC_ITF,packet,sizeof(packet));
    tinyusb_cdcacm_write_flush(PI_CDC_ITF,0);
}


bool sync_read_all_motors(uint32_t &elapsed_us)
{
    uint8_t rx_buffer[EXPECTED_RX_LEN];
    uart_flush_input(MOTOR_UART);    // Remove previous data.
    int64_t start_time = esp_timer_get_time();
    int written = uart_write_bytes(MOTOR_UART,SYNC_READ_PACKET,SYNC_READ_PACKET_LEN);
    if (written != SYNC_READ_PACKET_LEN)
    {return false;}
    // Wait until the complete request has been transmitted.
    if (uart_wait_tx_done(MOTOR_UART, pdMS_TO_TICKS(10)) != ESP_OK)
    {return false;}
    size_t received = 0;
    while (received < EXPECTED_RX_LEN)
    {
        int n = uart_read_bytes(MOTOR_UART,&rx_buffer[received],EXPECTED_RX_LEN - received,pdMS_TO_TICKS(20));
        if (n <= 0)
        {return false;}
        received += static_cast<size_t>(n);
    }

    int64_t end_time = esp_timer_get_time();
    elapsed_us = static_cast<uint32_t>(end_time - start_time);
    return true;
}


// Pi USB CDC RX callback
void pi_rx_callback(int itf,cdcacm_event_t *event)
{
    uint8_t rx[32];
    size_t rx_size = 0;
    if (tinyusb_cdcacm_read(itf,rx,sizeof(rx),&rx_size) != ESP_OK)
    {return;}
    for (size_t i = 0; i < rx_size; ++i)
    {
        if (rx[i] == 0x01) // constexpr uint8_t TRIGGER_READ_MOTORS = 0x01;
        {
            xTaskNotifyGive(benchmark_task_handle);
            break;
        }
    }
}

void benchmark_task(void *arg)
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        uint32_t elapsed_us = 0;
        bool result = sync_read_all_motors(elapsed_us);
        if (result)
        {send_time_to_pi(elapsed_us);}
        else
        {
            uint32_t error = 0xFFFFFFFF;
            send_time_to_pi(error);
        }
    }
}


extern "C" void app_main()
{
    // motor_uart_init
    uart_config_t config = {};
    config.baud_rate = MOTOR_BAUDRATE;
    config.data_bits = UART_DATA_8_BITS;
    config.parity = UART_PARITY_DISABLE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;

    ESP_ERROR_CHECK(uart_param_config(MOTOR_UART, &config));
    ESP_ERROR_CHECK(uart_set_pin(MOTOR_UART,MOTOR_TX_PIN,MOTOR_RX_PIN,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(MOTOR_UART,1024,1024,0,nullptr,0));

    // pi cdc init
    const tinyusb_config_cdcacm_t cdc_config =
    {PI_CDC_ITF,pi_rx_callback,nullptr,nullptr,nullptr};
    ESP_ERROR_CHECK(tinyusb_cdcacm_init(&cdc_config));

    xTaskCreate(benchmark_task,"benchmark_task",4096,nullptr,10,&benchmark_task_handle);
}