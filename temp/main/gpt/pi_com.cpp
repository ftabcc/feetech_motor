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

    TaskHandle_t packet_process_task_handle = nullptr;
    xTaskCreate(packet_process_task, "packet_process", 4096, nullptr, 10, &packet_process_task_handle);
}

// save
pi_comm pi_comm_instance;
void pi_comm::rx_callback(int itf,cdcacm_event_t *event)
{
    (void)event;
    int result = pi_comm_instance.rx_packet(itf); // 패킷수신까지만 callback안에 넣고, notify하는게 나은
    // 통신실패에 따른 처리
    switch (result)
    {
        case Comm_Result::SUCCESS:
        {
            xTaskNotifyGive(packet_process_task_handle);
            break;
        }
        case Comm_Result::FAIL:
            break;
        case Comm_Result::BUF_LEN_OVER:
            break; // pi로 전달
        case Comm_Result::BUF_NUM_OVER:
            break; // pi로 전달
        case Comm_Result::RX_CORRUPT:
            break; // 단순히 다음 패킷 기다리기
        case Comm_Result::RX_TIMEOUT:
            break; // 단순히 다음 패킷 기다리기
        case Comm_Result::CDC_ERR:
            break; // cdc실패 단순 pi로 전달.

        tx_packet(); // pi로 에러 전달
    }   
}


void pi_comm::rx_callback(int itf, cdcacm_event_t *event)
{
    (void)event;
    constexpr size_t RX_TEMP_SIZE = 64;
    uint8_t temp[RX_TEMP_SIZE];
    size_t rx_size = 0;
    esp_err_t ret = tinyusb_cdcacm_read(itf, temp, sizeof(temp), &rx_size);
    if (ret != ESP_OK)
    {return;} // CDC error handling
    if (rx_size == 0)
    {return;}
    if (!rx_ring_buffer.write(temp, rx_size))
    {return;}// RX ring buffer overflow
    xTaskNotifyGive(rx_task_handle);
}

void pi_comm::rx_task(void *arg)
{
    pi_comm *self = static_cast<pi_comm *>(arg);
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        while (self->rx_ring_buffer.available() > 0)
        {
            Comm_Result result = self->rx_packet();
            if (result == Comm_Result::NEED_MORE_DATA)
            {break;}
            if (result == Comm_Result::SUCCESS)
            {xTaskNotifyGive(self->packet_process_task_handle);}
        }
    }
}

Comm_Result pi_comm::rx_packet()
{
    constexpr uint16_t HEADER_LEN = 4;
    constexpr uint16_t MIN_PACKET_LEN = 11;
    uint8_t byte = 0;

    while (rx_ring_buffer.pop(byte))
    {
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
                    else if (byte == 0xFF)
                    {
                        // FF FF FF → 마지막 FF를 새로운 시작으로 사용
                        rx_parse_buffer[1] = 0xFF;
                        rx_parse_length = 2;
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
                    else if (byte == 0xFF)
                    {
                        // FF FF FD FF → 이 FF를 새로운 header 시작으로 사용
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
            rx_packet_len = static_cast<uint16_t>(byte) + 8;
            if (rx_packet_len < MIN_PACKET_LEN || rx_packet_len > RXPACKET_MAX_LEN) // invalid packet length
            {
                rx_parse_length = 0;
                rx_packet_len = 0;
                if (byte == 0xFF) // Current byte can be the start of a new header
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
            // 0x55 = reply instruction
            if (byte != 0x55)
            {
                rx_parse_length = 0;
                rx_packet_len = 0;
                if (byte == 0xFF) // Current byte can be the start of a new header
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
        if (rxpackets.count >= RXPACKET_MAX_NUM) // RX packet buffer full
        {
            rx_parse_length = 0;
            rx_packet_len = 0;
            return Comm_Result::BUF_NUM_OVER;
        }

        pi2esp_packet_t &rxpacket = rxpackets.packets[rxpackets.write_idx];
        rxpacket.data_len = rx_packet_len - 8;
        rxpacket.inst = rx_parse_buffer[PKT_INSTRUCTION];

        if (rxpacket.data_len > sizeof(rxpacket.data))
        {
            rx_parse_length = 0;
            rx_packet_len = 0;
            return Comm_Result::BUF_LEN_OVER;
        }

        memcpy(rxpacket.data,&rx_parse_buffer[6],rxpacket.data_len);
        Comm_Result result = unstuffing(rxpacket.data, &rxpacket.data_len);

        if (result != Comm_Result::SUCCESS)
        {
            rx_parse_length = 0;
            rx_packet_len = 0;
            return result;
        }

        rxpackets.write_idx = (rxpackets.write_idx + 1) % RXPACKET_MAX_NUM;
        rxpackets.count++;
        // Reset parser for next packet
        rx_parse_length = 0;
        rx_packet_len = 0;
        return Comm_Result::SUCCESS;
    }
    // Ring buffer became empty before a complete packet arrived
    return Comm_Result::NEED_MORE_DATA;
}


// save
int pi_comm::rx_packet(int itf)
{
    const uint16_t min_length       = 11;   // 프로토콜 구조상 될 수 있는 패킷의 최소 길이
    const uint16_t max_length = 255;    // temp버퍼의 최대 길이
    uint8_t temp[max_length];  // rx패킷을 찾기전에 잠시 저장하는 공간.
    uint16_t temp_length        = 0;    // 현재 temp버퍼 바이트 수
    uint16_t packet_len = 0;              // packet의 실제 길이

    size_t rx_size            = 0;     // cdc로 이번에 실제로 읽은 바이트 수
    uint16_t wait_length      = min_length;   // 지금 기다리는 전체 패킷 길이 (최소 상태패킷 길이로 시작)

    uint16_t idx              = 0;    // 이번에 확인하기 시작하는 바이트의 temp idx. idx이전은 헤더시작불가능 영역. 
    bool     found            = false;// 헤더패턴 찾음 여부
    bool     header_confirmed = false;// 헤더패턴 + 내용검증(Reserved+Length+Instruction) 검증 여부
    const uint16_t header_len = 3;   //  FF FF FD + byte stuffing 여부 바이트(FD면 byte-stuffing)
    int      result           = Comm_Result::FAIL; // 종류: COMM_SUCCESS, COMM_FAIL(default), [COMM_RX_CORRUPT, COMM_BUF_OVER, COMM_RX_TIMEOUT, COMM_CDC_ERR]

    if (rxpacket_buffer.count >= RXPACKET_MAX_NUM)
    {result Comm_Result::BUF_NUM_OVER;}
    else
    {
        while (true)
        {
            if (temp_length + wait_length > sizeof(temp))
            {
                result = Comm_Result::BUF_LEN_OVER;
                break;
            }
            esp_err_t ret = tinyusb_cdcacm_read(itf,&temp[temp_length],wait_length,&rx_size); // 어느CDC,어디저장,최대저장바이트수,실제읽은 바이트 어디저장
            if (ret != ESP_OK)
            {
                result = Comm_Result::CDC_ERR;
                break;
            }

            temp_length += rx_size;
            if (rx_size >= wait_length)
            {
                if (!header_confirmed)
                {
                    uint16_t limit = temp_length - header_len;
                    if (!found)
                    {
                        while (idx < limit) // limit이전까지만 헤더 확인 가능
                        {
                            uint8_t *p = (uint8_t *)memchr(&temp[idx], 0xFF, (size_t)(limit - idx)); // memchr(시작주소, 찾을값, 검색할바이트수);
                            if (p == nullptr)
                            {
                                idx = limit;   // 남은 구간에 0xFF 없으므로 더 볼 필요 없음
                                break;
                            }
                            idx = (uint16_t)(p - temp);
                            if ((temp[idx + 1] == 0xFF) &&(temp[idx + 2] == 0xFD) &&(temp[idx + 3] == 0x00))
                            {
                                found = true;
                                break;
                            }
                            idx += 1;
                        }
                        if (!found)
                        {
                            wait_length = idx + min_length - temp_length; // must be min_length
                            continue;
                        }
                    }
                    
                    if (found) // 헤더패턴 확인
                    {
                        if (temp[idx + PKT_RESERVED] != 0x00 // stuffing 
                            || temp[idx + PKT_LENGTH] + 8 > RXPACKET_MAX_LEN // packet_len = data(n) + 8
                            || temp[idx + PKT_INSTRUCTION] != 0x55) // 0x55 = reply inst
                        {
                            idx += header_len; // 헤더가 될수없는 범위에 대하여 skip
                            wait_length = idx + min_length - temp_length;
                            found = false;
                        }
                        else 
                        {
                            packet_len = temp[idx + PKT_LENGTH] + 8;
                            if (idx + packet_len > temp_length)// 실제 패킷 남은거 더 받아오게 
                            {wait_length = idx + packet_len - temp_length;}
                            header_confirmed = true;
                        }
                        continue;
                    }
                }
                if (header_confirmed)
                {
                    uint16_t crc = (temp[temp_length-1] & 0xFF) | ((temp[temp_length-2] & 0xFF) << 8); 
                    result = (updateCRC(0, &temp[idx], packet_len - 2) == crc) ? Comm_Result::SUCCESS : Comm_Result::RX_CORRUPT; // updateCRC(시작값, 시작주소, 검증길이)
                    if (result == Comm_Result::SUCCESS)
                    {
                        pi2esp_packet_t &rxpacket = rxpackets.packets[rxpackets.write_idx];
                        
                        memcpy(rxpacket.data, &temp[idx+6], packet_len-8); // HEAD(0xFF 0xFF 0xFD) + RSRV(!0xFD) + LEN(1) + INST(1) + DATA(N) + CRC(2)
                        rxpacket.data_len = packet_len - 8;
                        rxpacket.inst = temp[idx + PKT_INSTRUCTION];

                        result = unstuffing(rxpacket.data,&rxpacket.data_len);
                        if (result != Comm_Result::SUCCESS)
                        {break;}

                        rxpackets.write_idx = (rxpackets.write_idx + 1) % RXPACKET_MAX_NUM;
                        rxpackets.count++;
                    }
                    break;
                }
            }
            else    // temp_length < wait_length: 아직 필요한 만큼 못 받음.timeout 확인.
            {
                if (port->isPacketTimeout() == true)
                {
                    if (temp_length == 0)
                    {result = Comm_Result::RX_TIMEOUT;}
                    else
                    {result = Comm_Result::RX_CORRUPT;}
                    break;
                }
            }
            vTaskDelay(1);
        }
    }
    port->is_using_ = false;
    return result;
}

void pi_comm::packet_process_task(void *arg)
{
    pi_comm *self = static_cast<pi_comm *>(arg);
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);    // Wait until at least one packet is available
        while (self->rxpacket_buffer.count > 0)   // Process all queued packets
        {
            pi2esp_packet_t &packet = self->rxpacket_buffer.packets[self->rxpacket_buffer.read_idx];
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
                    write_packet(packet);
                    break;
                case INST_STOP:
                    break;
                default:
                    // Invalid instruction
                    break;
            }
            self->rxpacket_buffer.read_idx = (self->rxpacket_buffer.read_idx + 1) % PACKET_BUFFER_SIZE;
            self->rxpacket_buffer.count--;
        }
    }
}

void pi_comm::tx_task(void *arg)
{
    pi_comm *self = static_cast<pi_comm *>(arg);
    

}


static int pi_comm::tx_packet(int itf)
{
    tinyusb_cdcacm_write_queue(itf,data,len);
    tinyusb_cdcacm_write_flush(itf, 0);
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
            return COMM_FAIL;

        // Too short to contain FF FF FD
        if (*len < 3)
            return COMM_SUCCESS;

        // 1st pass: Count required stuffing bytes
        int stuffing_count = 0;
        for (int i = 0; i + 2 < *len; i++)
        {
            if (data[i] == 0xFF && data[i + 1] == 0xFF && data[i + 2] == 0xFD)
            {stuffing_count++;}
        }

        // No stuffing required
        if (stuffing_count == 0)
            return COMM_SUCCESS;

        // Check output buffer capacity
        if (*len + stuffing_count > RXPACKET_MAX_LEN - 8)
            return COMM_BUF_OVER;

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

    return COMM_SUCCESS;
}

int pi_comm::unstuffing(uint8_t *data, int *len)
{
    if (!skip_stuffing)
    {
        if (data == nullptr || len == nullptr || *len < 0)
            return COMM_FAIL;

        // Too short to contain FF FF FD FD
        if (*len < 4)
            return COMM_SUCCESS;

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

    return COMM_SUCCESS;
}