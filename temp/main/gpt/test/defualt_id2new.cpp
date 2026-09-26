#include <cstdint>
#include "driver/uart.h"
#include "esp_err.h"

namespace sts3215
{
    constexpr uint32_t DEFAULT_BAUDRATE = 1000000;

    constexpr uint8_t INST_PING  = 0x01;
    constexpr uint8_t INST_READ  = 0x02;
    constexpr uint8_t INST_WRITE = 0x03;

    constexpr uint8_t ADDR_ID   = 5;
    constexpr uint8_t ADDR_LOCK = 55;
    constexpr uint8_t ADDR_TORQUE_ENABLE = 40;

    constexpr uint8_t OLD_ID = 1;
    constexpr uint8_t NEW_ID = 5;

    uint8_t checksum(const uint8_t* packet, size_t length)
    {
        uint16_t sum = 0;

        // Sum ID, LENGTH, INSTRUCTION, PARAMETERS
        for (size_t i = 2; i < length; ++i)
        {
            sum += packet[i];
        }

        return static_cast<uint8_t>(~sum);
    }

    esp_err_t write_byte(uart_port_t uart_num, uint8_t id, uint8_t address, uint8_t value)
    {
        uint8_t packet[8];

        packet[0] = 0xFF;
        packet[1] = 0xFF;
        packet[2] = id;
        packet[3] = 0x04;       // Length = instruction + address + 1 byte data + checksum
        packet[4] = INST_WRITE;
        packet[5] = address;
        packet[6] = value;
        packet[7] = checksum(packet, 7);

        esp_err_t ret = uart_write_bytes(uart_num, packet, sizeof(packet));
        if (ret < 0)
        {
            return ESP_FAIL;
        }

        return uart_wait_tx_done(uart_num, pdMS_TO_TICKS(100));
    }

    esp_err_t change_id(uart_port_t uart_num)
    {
        // 1. Disable torque before changing EEPROM settings.
        esp_err_t ret = write_byte(uart_num, OLD_ID, ADDR_TORQUE_ENABLE, 0);
        if (ret != ESP_OK)
        {
            return ret;
        }

        // 2. Unlock EEPROM.
        ret = write_byte(uart_num, OLD_ID, ADDR_LOCK, 0);
        if (ret != ESP_OK)
        {
            return ret;
        }

        // 3. Change ID: 1 -> 5.
        ret = write_byte(uart_num, OLD_ID, ADDR_ID, NEW_ID);
        if (ret != ESP_OK)
        {
            return ret;
        }

        // 4. Lock EEPROM using the new ID.
        ret = write_byte(uart_num, NEW_ID, ADDR_LOCK, 1);
        if (ret != ESP_OK)
        {
            return ret;
        }

        return ESP_OK;
    }
}