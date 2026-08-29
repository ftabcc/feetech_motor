#ifndef ESP_SYSTEM_H
#define ESP_SYSTEM_H

#include <cstdint>
#include <cstddef>

#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"

#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tinyusb_cdc_acm.h"

#include "driver/uart.h"
#include "driver/i2c_master.h"
// what i made
#include "trajectory.h"

#define PACKET_MAX_LEN      100
#define RX_RINGBUF_SIZE     4096
#define RX_TASK_STACK       4096
#define RX_TASK_PRIORITY    10

// Packet structure for Pi communication
struct Packet {
    uint8_t buffer[PACKET_MAX_LEN + 3];
    size_t len = 0;
    size_t idx = 0;
};

// Pi Communication Module (TinyUSB CDC)
class PiComm {
public:
    esp_err_t init();
    void start();
    static void cdc_rx_callback(int itf, cdcacm_event_t *event);

private:
    static void rx_task(void *arg);
    static bool parse_packet(Packet *packet, uint8_t byte);
    static void handle_packet(const Packet &packet);

    static RingbufHandle_t rx_ringbuf;
    static TaskHandle_t rx_task_handle;
    static uint8_t rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE];
};

// IMU Subsystem (UART)
class ImuHandler {
public:
    esp_err_t init(uart_port_t port, int tx_pin, int rx_pin, uint32_t baud_rate);
    esp_err_t update();

private:
    uart_port_t uart_port = UART_NUM_MAX;
};

// Motor Driver Subsystem (UART)
class MotorDriver {
public:
    esp_err_t init(uart_port_t port, int tx_pin, int rx_pin, uint32_t baud_rate);
    esp_err_t send_command(uint8_t id, uint8_t cmd, const uint8_t *data, size_t len);

private:
    uart_port_t uart_port = UART_NUM_MAX;
};

// External Sensor Manager (I2C)
class SensorManager {
public:
    esp_err_t init(i2c_port_t port, int sda_pin, int scl_pin, uint32_t clk_speed);
    esp_err_t read_register(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len);

private:
    i2c_port_t i2c_port = I2C_NUM_MAX;
};

// Main System Controller Singleton
class ESPSystem {
public:
    static ESPSystem& getInstance() {
        static ESPSystem instance;
        return instance;
    }

    esp_err_t init();
    void start();

    // Module accessors
    PiComm& getPiComm() { return pi_comm; }
    ImuHandler& getImu() { return imu; }
    MotorDriver& getMotor() { return motor; }
    SensorManager& getSensorManager() { return sensor_mgr; }

private:
    ESPSystem() = default;
    ~ESPSystem() = default;
    ESPSystem(const ESPSystem&) = delete;
    ESPSystem& operator=(const ESPSystem&) = delete;

    PiComm pi_comm;
    ImuHandler imu;
    MotorDriver motor;
    SensorManager sensor_mgr;
};

#endif // ESP_SYSTEM_H