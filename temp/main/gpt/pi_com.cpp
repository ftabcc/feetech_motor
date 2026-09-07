#include "pi_com.h"

#include <assert.h>

#include "esp_log.h"
#include "esp_err.h"

#include "tinyusb.h"
#include "tusb_cdc_acm.h"

RingbufHandle_t pi_com::rx_ringbuf = NULL;
TaskHandle_t pi_com::rx_task_handle = NULL;
const char *pi_com::TAG = "PI_COM";
static uint8_t rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE];

static void pi_com::init(void *arg)
{
    rx_ringbuf = xRingbufferCreate(RX_RINGBUF_SIZE,RINGBUF_TYPE_BYTEBUF);
    assert(rx_ringbuf != NULL);

    BaseType_t ret_task = xTaskCreate(rx_task,"rx_task",RX_TASK_STACK,NULL,RX_TASK_PRIORITY,&rx_task_handle);
    assert(ret_task == pdPASS);

    ESP_LOGI(TAG, "USB initialization");
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
    ESP_LOGI(TAG, "USB initialization DONE");
}

static void pi_com::rx_task(void *arg)
{
    (void)arg; // for prevent warning: unused parameter 'arg'
    packet_t packet = {.idx = 0};
    while (1)
    {
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        while (1)
        {
            size_t item_size = 0;
            uint8_t *data = (uint8_t *)xRingbufferReceive(rx_ringbuf,&item_size,0);

            if (data == NULL)
            {break;}
            for (size_t i = 0; i < item_size; i++)
            {
                if (protocol::packet_parser(&packet,data[i]))
                    {pass}// switch case 하위함수
                // else
                //     {ESP_LOGW(TAG,"Invalid checksum");}
            }
            vRingbufferReturnItem(rx_ringbuf,(void *)data);
        }
    }
}


static void pi_com::tinyusb_cdc_rx_callback(int itf,cdcacm_event_t *event)
{
    (void)event;

    size_t rx_size = 0;
    esp_err_t ret = tinyusb_cdcacm_read(itf,rx_buf,sizeof(rx_buf),&rx_size); // 어느CDC,어디저장,최대저장바이트수,실제읽은 바이트 어디저장

    if (ret != ESP_OK)
    {ESP_LOGE(TAG, "tinyusb_cdcacm_read failed");
    return;}

    if (rx_size == 0)
    {return;}

    BaseType_t ret_rb = xRingbufferSend(rx_ringbuf,rx_buf,rx_size,0);

    if (ret_rb != pdTRUE)
    {ESP_LOGE(TAG,"RX Ring Buffer full. Data dropped: %u bytes",(unsigned)rx_size);
    return;}

    if (rx_task_handle != NULL)
    {xTaskNotifyGive(rx_task_handle);}
}


// 중간 링버퍼없이 바로 패킷확인
static int protocol::rxPacket(int itf)
{
  (void)event;

  const uint16_t HEADER_LEN = 3;   //  FF FF FD + byte stuffing 여부 바이트(FD면 byte-stuffing)
  int      result           = COMM_TX_FAIL; // 종류: COMM_TX_FAIL, COMM_SUCCESS, COMM_RX_CORRUPT
  size_t rx_size            = 0;     // cdc로 이번에 실제로 읽은 바이트 수
  uint16_t rx_length        = 0;    // 지금까지 확보한 유효 바이트 수 (write_idx 역할)
  uint16_t min_length       = 11;   // rxpacket의 프로토콜 구조상 될 수 있는 최소 길이
  uint16_t wait_length      = min_length;   // 지금 기다리는 전체 패킷 길이 (최소 상태패킷 길이로 시작)
  uint16_t idx              = 0;    // 이번에 확인하기 시작하는 바이트 idx. idx이전은 헤더시작불가능 영역. 
  bool     found            = false;// 헤더패턴을 확인
  bool     header_confirmed = false;// 헤더+Reserved+Length+Instruction 검증
  while (true)
  {
    esp_err_t ret = tinyusb_cdcacm_read(itf,&rxpacket[rx_length],wait_length - rx_length,&rx_size); // 어느CDC,어디저장,최대저장바이트수,실제읽은 바이트 어디저장

    if (ret != ESP_OK)
    {ESP_LOGE(TAG, "tinyusb_cdcacm_read failed");
    return;}
    if (rx_size == 0)
    {return;}
    
    rx_length += rx_size
    if (rx_length >= wait_length)
    {
      if (!header_confirmed)
      {
        uint16_t limit = rx_length - HEADER_LEN;
        if (found!)
        {
          while (idx < limit) // limit이전까지만 헤더 확인 가능
          {
            uint8_t *p = (uint8_t *)memchr(&rxpacket[idx], 0xFF, (size_t)(limit - idx)); // memchr(시작주소, 찾을값, 검색할바이트수);
            if (p == nullptr)
            {idx = limit;   // 남은 구간에 0xFF 없으므로 더 볼 필요 없음
              break;
            }
            //-----------------------------------여기까지 확인
            idx = (uint16_t)(p - rxpacket);
            if ((rxpacket[idx + 1] == 0xFF) &&(rxpacket[idx + 2] == 0xFD) &&(rxpacket[idx + 3] != 0xFD))
            {found = true;
              break;}
            idx += 1;
          }
          if (found!)
          {
            wait_length = idx + min_length;
            continue;
          }
        }
        
        if (found) // 헤더패턴 확인
        {
          if (rxpacket[idx + PKT_RESERVED] != 0x00 || rxpacket[idx + PKT_LENGTH] > RXPACKET_MAX_LEN || rxpacket[idx + PKT_INSTRUCTION] != 0x55) // 내용 검증
          {
            wait_length += HEADER_LEN;
            idx += HEADER_LEN;
            found = false;
            continue;
          }
          // 헤더패턴 + 내용 검증 후 = 진짜 헤더
          uint16_t real_len = rxpacket[idx + PKT_LENGTH] + PKT_LENGTH + 1;
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
        uint16_t crc = rxpacket[rx_length-1];
        result = (updateCRC(&rxpacket[idx], rx_length - idx) == crc) ? COMM_SUCCESS : COMM_RX_CORRUPT; // updateCRC(시작주소,검증길이)
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

