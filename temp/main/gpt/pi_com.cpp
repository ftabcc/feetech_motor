#include "pi_comm.h"

#include <assert.h>
#include "esp_log.h"
#include "esp_err.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"

RingbufHandle_t pi_comm::rx_ringbuf = NULL;
TaskHandle_t pi_comm::rx_task_handle = NULL;
const char *pi_comm::TAG = "PI_COM";
static uint8_t rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE];

static void pi_comm::init(void *arg)
{
    // ESP_LOGI(TAG, "USB initialization");
    const tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = {
        .cdc_port = TINYUSB_CDC_ACM_0,
        .callback_rx = &tinyusb_cdc_rx_callback,
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL
    };

    ESP_ERROR_CHECK(tinyusb_cdcacm_init(&acm_cfg));
    // ESP_LOGI(TAG, "USB initialization DONE");
}

static void pi_comm::tinyusb_cdc_rx_callback(int itf,cdcacm_event_t *event)
{
    (void)event;
    result = pi_comm::rxpacket(itf);
    if (result != COMM_SUCCESS){
        pass;
        // 통신결과에 따른 처리
    }
    switch (packet->inst)
    {
        case INST_READ:
            read_packet(packet);
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

static int pi_comm::rxpacket(int itf)
{
    const uint16_t min_length       = 11;   // temp버퍼의 프로토콜 구조상 될 수 있는 최소 길이
    const uint16_t max_length = 255;    // temp버퍼의 최대 길이
    uint16_t real_len = 0;              // packet의 실제 길이
    uint8_t temp[max_length]  = {};  // rx패킷을 찾기전에 잠시 저장하는 공간.

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
                uint16_t crc = temp[rx_length-1];
                result = (updateCRC(&temp[idx], rx_length - idx) == crc) ? COMM_SUCCESS : COMM_RX_CORRUPT; // updateCRC(시작주소,검증길이)
                memmove(&rxpacket[0], &temp[idx], real_len); // memmove(목적지 시작주소, 원본시작주소, 이동할바이트수);
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