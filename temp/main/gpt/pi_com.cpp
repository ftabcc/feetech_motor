#include "pi_comm.h"

#include <assert.h>
#include "esp_log.h"
#include "esp_err.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"

static void pi_comm::init(void *arg)
{
    // ESP_LOGI(TAG, "USB initialization");
    const tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = {
        .cdc_port = TINYUSB_CDC_ACM_0,
        .callback_rx = &rx_callback,
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL
    };

    ESP_ERROR_CHECK(tinyusb_cdcacm_init(&acm_cfg));

    rx_queue = xQueueCreate(pi_protocol::RXPACKET_MAX_NUM, sizeof(pi2esp_packet_t));
    tx_queue = xQueueCreate(pi_protocol::TXPACKET_MAX_NUM, sizeof(esp2pi_packet_t));

    xTaskCreate(rx_task, "rx_task", 4096, this, 10, &rx_task_handle);
    xTaskCreate(packet_process_task, "packet_process", 4096, this, 10, nullptr);
    xTaskCreate(tx_task, "tx_task", 4096, this, 10, nullptr);

}

void pi_comm::rx_callback(int itf, cdcacm_event_t *event)
{
    (void)event;
    constexpr size_t RX_TEMP_SIZE = 64;
    uint8_t temp[RX_TEMP_SIZE];
    size_t rx_size = 0;
    esp_err_t ret = tinyusb_cdcacm_read(itf, temp, sizeof(temp), &rx_size);
    if (ret != ESP_OK)
    {
        return;
    } 
    if (rx_size == 0)
    {
        return;
    }
    
    if (rx_parse_buffer.write(temp, rx_size) != rx_size)
    {
        // parse buffer overflow
        return;
    }
    if (rx_debug_buffer.write(temp, rx_size) != rx_size)
    {
        // timeout 처리?...??
    }
    xTaskNotifyGive(rx_task_handle);
}


void pi_comm::rx_task(void *arg)
{
    pi_comm *self = static_cast<pi_comm *>(arg);
    pi_protocol::rxpacket_t rxpacket;
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // '''timeout확인 필요'''
        //     if (self->rx_buffer_length > 0 &&
        //         self->port->isPacketTimeout())
        //     {
        //         self->rx_buffer_length = 0;
        //         self->rx_packet_len = 0;

        //         self->rx_debug_buffer.dump();
        //         self->rx_debug_buffer.clear();

        //         self->tx_packet();  // Send RX timeout error

        //         break;
        //     }

        while (self->rx_parse_buffer.available() > 0)
        {
            

            Comm_Result result = self->rx_packet(rxpacket;);
            if (result == Comm_Result::NEED_MORE_DATA)
            {break;}
            if (result == Comm_Result::SUCCESS)
            {
                self->rx_debug_buffer.clear()
                if (xQueueSend(self->rx_queue,&self->rxpacket,0) != pdTRUE) // (,,꽉 차면 대기할 시간)
                {
                    // RX packet queue full
                }
            }
            else
            {
                self->tx_packet();
            }
        }
    }
}

pi_protocol::Comm_Result pi_comm::rx_packet(pi_protocol::rxpacket_t &rxpacket)
{
    constexpr uint16_t HEADER_LEN = 4;
    constexpr uint16_t MIN_PACKET_LEN = 11;
    uint8_t byte = 0;

    while (rx_parse_buffer.read(byte)) // read one byte
    {
        rx_debug_buffer.write(&byte, 1);
        if (rx_parse_length < HEADER_LEN)
        {
            switch (rx_parse_length)
            {
                case 0:
                {
                    if (byte == 0xFF)
                    {
                        rx_parse_buffer[0] = byte;
                        rx_parse_length = 1;
                    }
                    break;
                }
                case 1:
                {
                    if (byte == 0xFF)
                    {
                        rx_parse_buffer[1] = byte;
                        rx_parse_length = 2;
                    }
                    else
                    {
                        rx_parse_length = 0;
                    }
                    break;
                }
                case 2:
                {
                    if (byte == 0xFD)
                    {
                        rx_parse_buffer[2] = byte;
                        rx_parse_length = 3;
                    }
                    else if (byte == 0xFF) // can be 2nd of a new header
                    {
                        break;
                    }
                    else
                    {
                        rx_parse_length = 0;
                    }
                    break;
                }
                case 3:
                {
                    if (byte == 0x00)
                    {
                        rx_parse_buffer[3] = byte;
                        rx_parse_length = 4;
                    }
                    else if (byte == 0xFF) // can be 1st of a new header
                    {
                        rx_parse_buffer[0] = 0xFF; 
                        rx_parse_length = 1;
                    }
                    else
                    {
                        rx_parse_length = 0;
                    }
                    break;
                }
            }
            continue;
        }

        // Read LENGTH
        if (rx_parse_length == 4)
        {
            rx_parse_buffer[4] = byte;
            rx_packet_len = static_cast<uint16_t>(byte) + 8; // rxpacket_len = data(n) + 8
            if (rx_packet_len < MIN_PACKET_LEN || rx_packet_len > pi_protocol::RXPACKET_MAX_LEN) // invalid packet length
            {
                rx_parse_length = 0;
                rx_packet_len = 0;
                if (byte == 0xFF) // can be 1st of a new header
                {
                    rx_parse_buffer[0] = byte;
                    rx_parse_length = 1;
                }
                continue;
            }
            rx_parse_length = 5;
            continue;
        }
        // Read INSTRUCTION
        if (rx_parse_length == 5)
        {
            if (byte != 0x55) // 0x55 = reply instruction
            {
                rx_parse_length = 0;
                rx_packet_len = 0;
                if (byte == 0xFF) // can be 1st of a new header
                {
                    rx_parse_buffer[0] = byte;
                    rx_parse_length = 1;
                }
                continue;
            }
            rx_parse_buffer[5] = byte;
            rx_parse_length = 6;
            continue;
        }

        if (rx_parse_length < rx_packet_len) // Read DATA + CRC
        {rx_parse_buffer[rx_parse_length++] = byte;}
        if (rx_parse_length < rx_packet_len)// Packet not complete yet
        {continue;}
        
        // CRC check
        uint16_t crc = static_cast<uint16_t>(rx_parse_buffer[rx_packet_len - 2]) | (static_cast<uint16_t>(rx_parse_buffer[rx_packet_len - 1]) << 8);
        uint16_t calculated_crc = updateCRC(0, rx_parse_buffer, rx_packet_len - 2);

        if (calculated_crc != crc)
        {
            rx_parse_length = 0;
            rx_packet_len = 0;
            return Comm_Result::RX_CORRUPT;
        }

        if (rx_packet_len - 8 > sizeof(rxpacket.data))
        {
            rx_parse_length = 0;
            rx_packet_len = 0;
            return Comm_Result::BUF_LEN_OVER;
        }

        rxpacket.data_len = rx_packet_len - 8;
        rxpacket.inst = rx_parse_buffer[PKT_INSTRUCTION];
        memcpy(rxpacket.data,&rx_parse_buffer[6],rxpacket.data_len);
        Comm_Result result = unstuffing(rxpacket.data, &rxpacket.data_len);
        if (result != Comm_Result::SUCCESS)
        {
            rx_parse_length = 0;
            rx_packet_len = 0;
            return result;
        }

        // Reset parser for next packet
        rx_parse_length = 0;
        rx_packet_len = 0;

        return Comm_Result::SUCCESS;
    }
}

void pi_comm::packet_process_task(void *arg)
{
    pi_comm *self = static_cast<pi_comm *>(arg);
    pi2esp_packet_t packet;

    while (true)
    {
        if (xQueueReceive(self->rx_queue, &packet, portMAX_DELAY) != pdTRUE)
        {continue;}

        switch (packet.inst)
        {
            case INST_REGISTER_TRAJECTORY:
            {
                trajectory_err_t err = self->trajectory.register_trajectory(packet);
                switch (err)
                {
                    case trajectory_err_t::SUCCESS:
                        break;

                    case trajectory_err_t::INVALID_LENGTH:
                        break;

                    case trajectory_err_t::INVALID_DURATION:
                        break;

                    case trajectory_err_t::BUFFER_FULL:
                        break;
                }

                break;
            }

            case INST_COMMAND:
                self->write_packet(packet);
                break;

            case INST_STOP:
                break;

            default:
                // Invalid instruction
                break;
        }
    }
}

void pi_comm::tx_task(void *arg)
{
    pi_comm *self = static_cast<pi_comm *>(arg);
    pi_protocol::txpacket_t txpacket;
    while (true)
    {
        if (xQueueReceive(self->tx_queue, &txpacket, portMAX_DELAY) == pdTRUE)
        {self->tx_packet(&txpacket);}
    }
}

pi_protocol::Comm_Result pi_comm::tx_packet(pi_protocol::txpacket_t &txpacket)
{
    uint8_t tx_buffer[pi_protocol::TXPACKET_MAX_LEN];
    uint16_t tx_len = 0;

    Comm_Result result = stuffing(txpacket.data, &txpacket.data_len);
    if (result != Comm_Result::SUCCESS)
    {return result;}
        
    // 유의미한 조건문인가??...
    if (txpacket.data_len > sizeof(txpacket.data))
    {
        return Comm_Result::BUF_LEN_OVER;
    }

    tx_buffer[0] = 0xFF;
    tx_buffer[1] = 0xFF;
    tx_buffer[2] = 0xFD;
    tx_buffer[3] = 0x00;
    tx_buffer[4] = static_cast<uint8_t>(txpacket.data_len);
    tx_buffer[5] = txpacket.inst;
    tx_buffer[6] = txpacket.err;
    memcpy(&tx_buffer[7],txpacket.data,txpacket.data_len);

    // CRC
    uint16_t crc = updateCRC(0,tx_buffer,txpacket.data_len+7); // txpacket_len = data(n) + 9
    tx_buffer[txpacket.data_len+7] = static_cast<uint8_t>(crc & 0xFF);
    tx_buffer[txpacket.data_len+8] = static_cast<uint8_t>((crc >> 8) & 0xFF);

    if (tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, tx_buffer, txpacket.data_len+9) != ESP_OK)
    {return Comm_Result::CDC_ERR;}
    tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
    return Comm_Result::SUCCESS;
}

// CRC16bit(0x8005)
unsigned short pi_comm::updateCRC(uint16_t start, uint8_t *addr, uint16_t size)
{
  uint16_t i;
  static const uint16_t crc_table[256] = {0x0000,
  0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011, 0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027,
  0x0022, 0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D, 0x8077, 0x0072, 0x0050, 0x8055, 0x805F, 0x005A, 0x804B,
  0x004E, 0x0044, 0x8041, 0x80C3, 0x00C6, 0x00CC, 0x80C9, 0x00D8, 0x80DD, 0x80D7, 0x00D2, 0x00F0, 0x80F5, 0x80FF,
  0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1, 0x00A0, 0x80A5, 0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1, 0x8093,
  0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082, 0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197,
  0x0192, 0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE, 0x01A4, 0x81A1, 0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB,
  0x01FE, 0x01F4, 0x81F1, 0x81D3, 0x01D6, 0x01DC, 0x81D9, 0x01C8, 0x81CD, 0x81C7, 0x01C2, 0x0140, 0x8145, 0x814F,
  0x014A, 0x815B, 0x015E, 0x0154, 0x8151, 0x8173, 0x0176, 0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162, 0x8123,
  0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132, 0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104,
  0x8101, 0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D, 0x8317, 0x0312, 0x0330, 0x8335, 0x833F, 0x033A, 0x832B,
  0x032E, 0x0324, 0x8321, 0x0360, 0x8365, 0x836F, 0x036A, 0x837B, 0x037E, 0x0374, 0x8371, 0x8353, 0x0356, 0x035C,
  0x8359, 0x0348, 0x834D, 0x8347, 0x0342, 0x03C0, 0x83C5, 0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1, 0x83F3,
  0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2, 0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7,
  0x03B2, 0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E, 0x0384, 0x8381, 0x0280, 0x8285, 0x828F, 0x028A, 0x829B,
  0x029E, 0x0294, 0x8291, 0x82B3, 0x02B6, 0x02BC, 0x82B9, 0x02A8, 0x82AD, 0x82A7, 0x02A2, 0x82E3, 0x02E6, 0x02EC,
  0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2, 0x02D0, 0x82D5, 0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1, 0x8243,
  0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252, 0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264,
  0x8261, 0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E, 0x0234, 0x8231, 0x8213, 0x0216, 0x021C, 0x8219, 0x0208,
  0x820D, 0x8207, 0x0202 };

  for (uint16_t j = 0; j < data_blk_size; j++)
  {
    i = ((uint16_t)(crc_accum >> 8) ^ *data_blk_ptr++) & 0xFF;
    crc_accum = (crc_accum << 8) ^ crc_table[i];
  }

  return crc_accum;
}

int pi_comm::stuffing(uint8_t *data, int *len)
{   // 1차 탐색 2차 뒤부터 채우기(다이나믹셀 방식) vs 탐색하며 별도메모리에 추가하며 채우기
    if (!skip_stuffing)
    {
        
        if (data == nullptr || len == nullptr)
            return Comm_Result::FAIL;

        // Too short to contain FF FF FD
        if (*len < 3)
            return Comm_Result::SUCCESS;

        // 1st pass: Count required stuffing bytes
        int stuffing_count = 0;
        for (int i = 0; i + 2 < *len; i++)
        {
            if (data[i] == 0xFF && data[i + 1] == 0xFF && data[i + 2] == 0xFD)
            {stuffing_count++;}
        }

        // No stuffing required
        if (stuffing_count == 0)
            return Comm_Result::SUCCESS;

        // Check output buffer capacity
        if (*len + stuffing_count > pi_protocol::RXPACKET_MAX_LEN - 8)
            return Comm_Result::BUF_LEN_OVER;

        int read_idx = *len - 1;
        int write_idx = *len + stuffing_count - 1;

        // 2nd pass: Move data backward and insert FD
        while (read_idx >= 0)
        {
            // FF FF FD
            if (read_idx >= 2 && data[read_idx - 2] == 0xFF && data[read_idx - 1] == 0xFF && data[read_idx] == 0xFD)
            {
                // Add stuffing byte
                data[write_idx--] = 0xFD;

                // Move original FF FF FD
                data[write_idx--] = data[read_idx--]; // FD
                data[write_idx--] = data[read_idx--]; // FF
                data[write_idx--] = data[read_idx--]; // FF
            }
            else
            {data[write_idx--] = data[read_idx--];}// Move normal byte
        }

        // Update length
        *len += stuffing_count;
    }

    return Comm_Result::SUCCESS;
}

int pi_comm::unstuffing(uint8_t *data, int *len)
{
    if (!skip_stuffing)
    {
        if (data == nullptr || len == nullptr || *len < 0)
            return Comm_Result::FAIL;

        // Too short to contain FF FF FD FD
        if (*len < 4)
            return Comm_Result::SUCCESS;

        int read_idx = 0;
        int write_idx = 0;

        // Read from front and compact in-place
        while (read_idx < *len)
        {
            // FF FF FD FD
            if (read_idx + 3 < *len && data[read_idx] == 0xFF && data[read_idx + 1] == 0xFF && data[read_idx + 2] == 0xFD && data[read_idx + 3] == 0xFD)
            {
                // Copy original FF FF FD
                data[write_idx++] = data[read_idx++];
                data[write_idx++] = data[read_idx++];
                data[write_idx++] = data[read_idx++];
                read_idx++; // Skip stuffed FD
            }
            else
            {data[write_idx++] = data[read_idx++];} // Move normal byte
        }

        // Update length
        *len = write_idx;
    }

    return Comm_Result::SUCCESS;
}