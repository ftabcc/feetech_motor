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
    // ESP_LOGI(TAG, "USB initialization DONE");
}

static void pi_comm::rx_callback(int itf,cdcacm_event_t *event)
{
    (void)event;
    result = pi_comm::rxpacket(itf);
    if (result != COMM_SUCCESS){
        pass;
        // 통신실패에 따른 처리
    }
    else
    {
        switch (rxpacket.inst)
        {
            case INST_CONTROL:
                inst::register_joint_trajectory(packet);
                break;

            case INST_WRITE:
                write_packet(packet);
                break;

            case INST_STATUS:
                send_status(packet);
                break;

            default:
                // 잘못된 instruction
                break;
        }
    }    
}

int pi_comm::rx_packet(int itf)
{
    const uint16_t min_length       = 11;   // temp버퍼의 프로토콜 구조상 될 수 있는 최소 길이
    const uint16_t max_length = 255;    // temp버퍼의 최대 길이
    uint16_t real_len = 0;              // packet의 실제 길이
    uint8_t temp[max_length];  // rx패킷을 찾기전에 잠시 저장하는 공간.

    size_t rx_size            = 0;     // cdc로 이번에 실제로 읽은 바이트 수
    uint16_t rx_length        = 0;    // 현재 temp버퍼 바이트 수
    uint16_t wait_length      = min_length;   // 지금 기다리는 전체 패킷 길이 (최소 상태패킷 길이로 시작)

    uint16_t idx              = 0;    // 이번에 확인하기 시작하는 바이트 idx. idx이전은 헤더시작불가능 영역. 
    bool     found            = false;// 헤더패턴 찾음 여부
    bool     header_confirmed = false;// 헤더패턴 + 내용검증(Reserved+Length+Instruction) 검증 여부
    const uint16_t HEADER_LEN = 3;   //  FF FF FD + byte stuffing 여부 바이트(FD면 byte-stuffing)
    int      result           = COMM_FAIL; // 종류: COMM_SUCCESS, COMM_FAIL(default), [COMM_RX_CORRUPT, COMM_BUF_OVER, COMM_RX_TIMEOUT, COMM_CDC_ERR]

    while (true)
    {
        if (wait_length > max_length)
        {
            result = COMM_BUF_OVER;
            break;
        }
        esp_err_t ret = tinyusb_cdcacm_read(itf,&temp[rx_length],wait_length - rx_length,&rx_size); // 어느CDC,어디저장,최대저장바이트수,실제읽은 바이트 어디저장
        if (ret != ESP_OK)
        {
            result = COMM_CDC_ERR;
            break;
        }

        rx_length += rx_size;
        if (rx_length >= wait_length)
        {
            if (!header_confirmed)
            {
                uint16_t limit = rx_length - HEADER_LEN;
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
                    if ((temp[idx + 1] == 0xFF) &&(temp[idx + 2] == 0xFD) &&(temp[idx + 3] != 0xFD))
                    {
                        found = true;
                        break;
                    }
                    idx += 1;
                }
                if (!found)
                {
                    wait_length = idx + min_length;
                    continue;
                }
                }
                
                if (found) // 헤더패턴 확인
                {
                if (temp[idx + PKT_RESERVED] != 0x00 || temp[idx + PKT_LENGTH] > RXPACKET_MAX_LEN || temp[idx + PKT_INSTRUCTION] != 0x55) // 내용 검증
                {
                    wait_length += HEADER_LEN;
                    idx += HEADER_LEN; // 헤더가 될수없는 범위에 대하여 skip
                    found = false;
                    continue;
                }
                // 헤더패턴 + 내용 검증 후 = 진짜 헤더
                uint16_t real_len = temp[idx + PKT_LENGTH] + PKT_LENGTH + 1;
                if (idx + real_len > rx_length)// 실제 패킷 남은거 더 받아오게 
                {
                    wait_length = idx + real_len;
                    header_confirmed = true;
                    continue;
                }
                header_confirmed = true;
                }
            }
            if (header_confirmed)
            {
                uint16_t crc = (temp[rx_length-1] & 0xFF) | ((temp[rx_length-2] & 0xFF) << 8); 
                result = (updateCRC(0, &temp[idx], rx_length - idx) == crc) ? COMM_SUCCESS : COMM_RX_CORRUPT; // updateCRC(시작값, 시작주소, 검증길이)
                if (result == COMM_SUCCESS)
                {
                    if (!skip_stuffing)
                    {removeStuffing(rxpacket);}
                    rxpacket.len = temp[idx + PKT_LENGTH];
                    rxpacket.inst = temp[idx + PKT_INSTRUCTION];
                    memmove(rxpacket.data, &temp[idx+7], real_len-8); // HEAD(0xFF 0xFF 0xFD) + RSRV(!0xFD) + LEN(1) + INST(1) + DATA(N) + CRC(2)
                }
                break;
            }
        }
        else    // rx_length < wait_length: 아직 필요한 만큼 못 받음.timeout 확인.
        {
            if (port->isPacketTimeout() == true)
            {
                if (rx_length == 0)
                {result = COMM_RX_TIMEOUT;}
                else
                {result = COMM_RX_CORRUPT;}
                break;
            }
        }
        // #if defined(ESP_PLATFORM)
        // taskYIELD()는 "같은 우선순위의 다른 태스크"에게만 양보한다. 이 태스크가 idle보다 높은 우선순위에서 계속 ready 상태라면 idle 태스크(및 watchdog feed)가 전혀 실행되지
        // 못해 Task Watchdog reset을 유발할 수 있다. 확실히 안전하려면 최소 1 tick의 실제 delay가 필요하다. 다만 이 delay는 (특히 half-duplex 응답을 기다리는 구간에서) 왕복
        // 타이밍에 영향을 줄 수 있으므로, tick rate를 충분히 높이거나(예: 1kHz), 응답을 이미 기다리는 도중이 아니라 유휴 구간에서만 이 폴링이 자주 발생하도록 상위 타임아웃 설계와 맞춰 사용하는 것을 권장한다.
        vTaskDelay(1);
    }

    port->is_using_ = false;
    return result;
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