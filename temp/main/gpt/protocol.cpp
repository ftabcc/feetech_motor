#include "protocol.h"

// 지금은 패킷검색시 계속 불필요바이트들이 있을때 마다 밀어가며 사용하는데, 밀어가면서 하지 말고 긴 버퍼에서 현재까지 확인한 idx를 변수로 두는게 나은거 같다.
int Protocol2PacketHandler::rxPacket(PortHandler *port, uint8_t *rxpacket, bool skip_stuffing)
{
  int     result         = COMM_TX_FAIL;
  uint16_t rx_length     = 0;
  uint16_t wait_length   = 11; // minimum length (HEADER0 HEADER1 HEADER2 RESERVED ID LENGTH_L LENGTH_H INST ERROR CRC16_L CRC16_H)

  while(true)
  {
    rx_length += port->readPort(&rxpacket[rx_length], wait_length - rx_length);
    if (rx_length >= wait_length)
    {
      uint16_t idx = 0;

      // find packet header 헤더검색 성공후 쓸모없는 바이트 삭제후 다시 돌아온 경우엔 헤더검색 skip하게 하기.
      for (idx = 0; idx < (rx_length - 3); idx++)
      {
        if ((rxpacket[idx] == 0xFF) && (rxpacket[idx+1] == 0xFF) && (rxpacket[idx+2] == 0xFD) && (rxpacket[idx+3] != 0xFD)) // rxpacket[idx+3] != 0xFD는 byte stuffing검사
          break;
      }

      if (idx == 0)   // found at the beginning of the packet
      {
        if (rxpacket[PKT_RESERVED] != 0x00 ||
           DXL_MAKEWORD(rxpacket[PKT_LENGTH_L], rxpacket[PKT_LENGTH_H]) > RXPACKET_MAX_LEN ||
           rxpacket[PKT_INSTRUCTION] != 0x55)// rxpacket[PKT_ID] > 0xFC || // FAST protocol responds with a broadcast ID
        {
          // remove the first byte in the packet 가짜헤더를 찾은경우엔 가짜헤더 일부가 진짜헤더가 될 수없으니 가짜헤더 바이트 만큼 삭제하기.
          for (uint16_t s = 0; s < rx_length - 1; s++)
            rxpacket[s] = rxpacket[1 + s];
          //memcpy(&rxpacket[0], &rxpacket[idx], rx_length - idx);
          rx_length -= 1;
          continue;
        }

        // re-calculate the exact length of the rx packet
        if (wait_length != DXL_MAKEWORD(rxpacket[PKT_LENGTH_L], rxpacket[PKT_LENGTH_H]) + PKT_LENGTH_H + 1)
        {
          wait_length = DXL_MAKEWORD(rxpacket[PKT_LENGTH_L], rxpacket[PKT_LENGTH_H]) + PKT_LENGTH_H + 1;
          continue;
        }

        if (rx_length < wait_length) // dead-code, need to del
        {
          // check timeout
          if (port->isPacketTimeout() == true)
          {
            if (rx_length == 0)
            {result = COMM_RX_TIMEOUT;}
            else
            {result = COMM_RX_CORRUPT;}
            break;
          }
          else
          {continue;}
        }

        // verify CRC16
        uint16_t crc = DXL_MAKEWORD(rxpacket[wait_length-2], rxpacket[wait_length-1]);
        if (updateCRC(0, rxpacket, wait_length - 2) == crc)
        {result = COMM_SUCCESS;}
        else
        {result = COMM_RX_CORRUPT;}
        break;
      }
      else
      {
        // remove unnecessary packets 만약에 아예 헤더를 검색하지 못했다면 헤더바이트수만큼만 남기고 있는데, 헤더바이트수-1만큼만 남기는게 나은거 아닌가?
        for (uint16_t s = 0; s < rx_length - idx; s++)
          rxpacket[s] = rxpacket[idx + s];
        // memmove(rxpacket, rxpacket + idx, rx_length - idx);
        // memcpy(&rxpacket[0], &rxpacket[idx], rx_length - idx);
        rx_length -= idx;
      }
    }
    else
    {
      // check timeout
      if (port->isPacketTimeout() == true)
      {
        if (rx_length == 0)
        {result = COMM_RX_TIMEOUT;}
        else
        {result = COMM_RX_CORRUPT;}
        break;
      }
    }
    #if defined(__linux__) || defined(__APPLE__)
        usleep(0);
    #elif defined(_WIN32) || defined(_WIN64)
        Sleep(0);
    #endif
    }
  port->is_using_ = false;

  if ((result == COMM_SUCCESS) && (false == skip_stuffing))
    removeStuffing(rxpacket);

  return result;
}

// my fix
int protocol::rxPacket(packet_t *packet,uint8_t byte)
{
  int     result         = COMM_TX_FAIL;
  uint16_t rx_length     = 0;
  uint16_t wait_length   = 11; // minimum length (HEADER0 HEADER1 HEADER2 RESERVED ID LENGTH_L LENGTH_H INST ERROR CRC16_L CRC16_H)

  while(true)
  {
    rx_length += port->readPort(&rxpacket[rx_length], wait_length - rx_length);
    if (rx_length >= wait_length)
    {
      uint16_t idx = 0;

      // find packet header 헤더검색 성공후 쓸모없는 바이트 삭제후 다시 돌아온 경우엔 헤더검색 skip하게 하기.
      for (idx = 0; idx < (rx_length - 3); idx++)
      {
        if ((rxpacket[idx] == 0xFF) && (rxpacket[idx+1] == 0xFF) && (rxpacket[idx+2] == 0xFD) && (rxpacket[idx+3] != 0xFD)) // rxpacket[idx+3] != 0xFD는 byte stuffing검사
          break;
      }

      if (idx == 0)   // found at the beginning of the packet
      {
        if (rxpacket[PKT_RESERVED] != 0x00 ||
           DXL_MAKEWORD(rxpacket[PKT_LENGTH_L], rxpacket[PKT_LENGTH_H]) > RXPACKET_MAX_LEN ||
           rxpacket[PKT_INSTRUCTION] != 0x55)// rxpacket[PKT_ID] > 0xFC || // FAST protocol responds with a broadcast ID
        {
          // remove the first byte in the packet 가짜헤더를 찾은경우엔 가짜헤더 일부가 진짜헤더가 될 수없으니 가짜헤더 바이트 만큼 삭제하기.
          for (uint16_t s = 0; s < rx_length - 1; s++)
            rxpacket[s] = rxpacket[1 + s];
          //memcpy(&rxpacket[0], &rxpacket[idx], rx_length - idx);
          rx_length -= 1;
          continue;
        }

        // re-calculate the exact length of the rx packet
        if (wait_length != DXL_MAKEWORD(rxpacket[PKT_LENGTH_L], rxpacket[PKT_LENGTH_H]) + PKT_LENGTH_H + 1)
        {
          wait_length = DXL_MAKEWORD(rxpacket[PKT_LENGTH_L], rxpacket[PKT_LENGTH_H]) + PKT_LENGTH_H + 1;
          continue;
        }

        if (rx_length < wait_length) // dead-code, need to del
        {
          // check timeout
          if (port->isPacketTimeout() == true)
          {
            if (rx_length == 0)
            {result = COMM_RX_TIMEOUT;}
            else
            {result = COMM_RX_CORRUPT;}
            break;
          }
          else
          {continue;}
        }

        // verify CRC16
        uint16_t crc = DXL_MAKEWORD(rxpacket[wait_length-2], rxpacket[wait_length-1]);
        if (updateCRC(0, rxpacket, wait_length - 2) == crc)
        {result = COMM_SUCCESS;}
        else
        {result = COMM_RX_CORRUPT;}
        break;
      }
      else
      {
        // remove unnecessary packets 만약에 아예 헤더를 검색하지 못했다면 헤더바이트수만큼만 남기고 있는데, 헤더바이트수-1만큼만 남기는게 나은거 아닌가?
        for (uint16_t s = 0; s < rx_length - idx; s++)
          rxpacket[s] = rxpacket[idx + s];
        // memmove(rxpacket, rxpacket + idx, rx_length - idx);
        // memcpy(&rxpacket[0], &rxpacket[idx], rx_length - idx);
        rx_length -= idx;
      }
    }
    else
    {
      // check timeout
      if (port->isPacketTimeout() == true)
      {
        if (rx_length == 0)
        {result = COMM_RX_TIMEOUT;}
        else
        {result = COMM_RX_CORRUPT;}
        break;
      }
    }
    #if defined(__linux__) || defined(__APPLE__)
        usleep(0);
    #elif defined(_WIN32) || defined(_WIN64)
        Sleep(0);
    #endif
    }
  port->is_using_ = false;

  if ((result == COMM_SUCCESS) && (false == skip_stuffing))
    removeStuffing(rxpacket);

  return result;
}


// ============================================================================
// Protocol2PacketHandler::rxPacket() - 개선판
//
// 빌드 전 확인 사항:
//   - #include <cstring>   (memchr, memmove) — 원본은 memmove를 주석 처리만 하고
//     실제로는 안 쓰고 있었으므로, 이 include가 소스 파일에 없다면 추가해야 합니다.
//   - ESP32/FreeRTOS 분기를 쓰는 경우
//       #include "freertos/FreeRTOS.h"
//       #include "freertos/task.h"
//
// ----------------------------------------------------------------------------
// 원본과의 동작 차이 요약 (자세한 증명은 별도로 전달드린 분석글 참고)
//
//  1) 헤더 탐색 방식
//     4중 byte-compare 수동 루프 -> memchr 기반 스캔.
//     매칭 결과(어떤 idx를 찾는지)는 원본과 100% 동일, 구현 방식만 다름.
//
//  2) "헤더를 아예 못 찾음" 시 보존 바이트 수
//     결론: 원본과 동일하게 HEADER_LEN(3바이트) 보존.
//     (HEADER_LEN-1(2바이트)로 줄이면, 마지막 3바이트가 정확히 FF FF FD인 채로
//      스터핑 판정 보류 중이던 경우 앞의 FF를 잃어버려 정상 헤더를 통째로
//      놓치는 데이터 손실이 생김 - 반례로 증명됨)
//     실제로는 원본도 이미 3바이트를 보존하고 있었으므로 "동작 변화 없음",
//     memmove()를 실제로 호출하도록 구현 방식만 정리함.
//
//  3) 헤더 패턴은 맞지만 Reserved/Length/Instruction 검증 실패 시 폐기 바이트 수
//     1바이트 -> 3바이트로 변경.
//     증명: 후보 위치 [idx, idx+3] 중 idx+1, idx+2는 앞으로 어떤 데이터가 와도
//     헤더 시작이 될 수 없음이 이미 버퍼에 있는 값만으로 확정되고(byte stuffing
//     판정에 쓰인 rxpacket[idx+2]==0xFD 이므로), idx+3만 "0xFD가 아니다"라는
//     정보만 있을 뿐 미상이라 보존해야 함. => 3바이트 폐기가 유실 없는 안전한
//     최대치. **이 항목이 원본과 유일하게 실질적으로 다른 동작입니다.**
//
//  4) `if (rx_length < wait_length) {...}` (원본의 dead code) 제거
//     이 지점 도달 시 바깥 if(rx_length>=wait_length)가 이미 참이고, 그 사이
//     rx_length/wait_length가 전혀 바뀌지 않았으므로 항상 거짓인 코드였음.
//     readPort()/timeout 처리는 바깥 else 분기가 전담하므로 동작 변화 없음.
//
//  5) usleep(0)/Sleep(0) 유지 + ESP32/FreeRTOS 분기 신규 추가.
//     기존 POSIX/Windows 동작은 그대로 두고, ESP32에서는 taskYIELD()의 한계
//     (idle 태스크/워치독을 굶길 수 있음)를 피하기 위해 vTaskDelay(1) 사용 권장.
//
//  6) search_idx 변수 도입
//     "필요한 만큼만 읽는" 현재 read 정책 하에서는 매 탐색 진입 시 항상 0으로
//     리셋되어(모든 분기가 버퍼를 수정하며 0으로 되돌림) 교차-호출 재탐색 방지
//     효과는 제한적입니다. 다만 구조를 이렇게 잡아두면, 만약 나중에 read 정책을
//     "가능한 만큼 크게 읽기"로 바꾸더라도(별도 검증 필요 - 아래 참고) 코드 변경
//     없이 자동으로 재탐색 방지 효과를 얻습니다.
//
//  참고(선택적 추가 개선, 이번 변경에는 미포함):
//     readPort()에 매번 wait_length-rx_length만큼만 요청하는 대신 버퍼 capacity
//     까지 한 번에 크게 읽어오면 read 호출 자체의 횟수를 줄일 수 있습니다.
//     다만 PortHandler::readPort()/isPacketTimeout()가 "요청 길이"에 의존하는
//     내부 동작(예: 블로킹 대기 시간 산정)을 갖고 있을 수 있으므로, 이 부분은
//     반드시 실제 PortHandler 구현을 먼저 확인한 뒤 적용해야 합니다.
// ============================================================================

int Protocol2PacketHandler::rxPacket(PortHandler *port, uint8_t *rxpacket, bool skip_stuffing)
{
  const uint16_t HEADER_LEN = 3;   // 진짜 헤더 FF FF FD (3바이트).
                                    // 4번째 바이트는 패턴이 아니라 byte stuffing 여부를
                                    // 가리는 lookahead 바이트일 뿐, 프로토콜 의미는 원본과 동일.

  int      result           = COMM_TX_FAIL;
  uint16_t rx_length        = 0;    // 지금까지 확보한 유효 바이트 수 (write_idx 역할)
  uint16_t wait_length      = 11;   // 지금 기다리는 전체 패킷 길이 (최소 상태패킷 길이로 시작)
  uint16_t search_idx       = 0;    // [0, search_idx) 구간은 "헤더가 시작될 수 없다"고 이미 결론난 영역
  bool     header_confirmed = false;// 헤더+Reserved+Length+Instruction 검증이 끝나면 true

  while (true)
  {
    // 원본과 동일한 read 정책: 지금 기다리는 길이(wait_length)를 채우는 데 필요한 만큼만 읽는다.
    // readPort()/isPacketTimeout()의 동작에 영향을 줄 수 있어 이 부분은 그대로 둔다.
    rx_length += port->readPort(&rxpacket[rx_length], wait_length - rx_length);

    if (rx_length >= wait_length)
    {
      if (!header_confirmed)
      {
        // ---------------- 헤더 탐색 ----------------
        // 후보 시작 idx가 "확정 판정" 되려면 idx+3까지 실제 데이터가 있어야 한다
        // (FF FF FD 3바이트 + byte stuffing 판정용 4번째 바이트).
        uint16_t idx   = search_idx;
        uint16_t limit = rx_length - HEADER_LEN;   // idx는 [search_idx, limit) 범위에서만 확정 판정 가능
        bool     found = false;

        while (idx < limit)
        {
          // 0xFF가 아닌 바이트를 1바이트씩 비교하는 원본 방식 대신, memchr로 다음 0xFF 위치까지
          // 한 번에 건너뛴다. 매칭 조건 자체(FF FF FD + stuffing 검사)는 원본과 완전히 동일하다.
          uint8_t *p = (uint8_t *)memchr(&rxpacket[idx], 0xFF, (size_t)(limit - idx));
          if (p == nullptr)
          {
            idx = limit;   // 남은 구간에 0xFF조차 없으므로 더 볼 필요 없음
            break;
          }
          idx = (uint16_t)(p - rxpacket);

          if ((rxpacket[idx + 1] == 0xFF) &&(rxpacket[idx + 2] == 0xFD) &&(rxpacket[idx + 3] != 0xFD))   // byte stuffing 검사: 원본 조건과 100% 동일
          {found = true;
            break;}
          idx += 1;   // 이 0xFF는 헤더 시작이 아니었다. 그 다음 바이트부터 계속 탐색.
        }

        if (!found)
        {
          // [증명] idx가 limit까지 조건을 만족하지 못했다는 것은, 0 ~ limit-1의 모든 시작 위치가
          // "지금 가진 데이터만으로 완전히" 기각되었다는 뜻이다. 반면 마지막 HEADER_LEN(3)바이트
          // [limit, rx_length)는 4번째(byte stuffing 판정) 바이트가 아직 도착하지 않아
          // "완전 기각"이 아니라 "판정 보류" 상태이므로 반드시 보존해야 한다.
          //
          // HEADER_LEN-1(2바이트)로 줄이면: 마지막 3바이트가 정확히 FF FF FD인 채로
          // 보류된 경우 앞의 FF 하나를 잃어버려, 다음에 stuffing이 아닌 4번째 바이트가
          // 도착해도 FF FF FD 패턴을 다시 만들 수 없다 -> 실재하는 정상 헤더를 놓치는
          // 데이터 손실. 따라서 HEADER_LEN(3) 보존이 유실 없는 "정확한" 최소 보존량이다.
          if (limit > 0)
            memmove(&rxpacket[0], &rxpacket[limit], HEADER_LEN);
          rx_length  = HEADER_LEN;
          search_idx = 0;
          // 원본처럼 이 경로는 새 데이터가 필요하므로 아래로 흘러가 yield 후 다시 읽는다.
        }
        else if (idx > 0)
        {
          // [증명] idx 이전 구간은 위 루프에서 이미 "헤더가 될 수 없음"이 전부 확정 판정된
          // 상태이므로, 후보를 찾을 때마다/바이트마다가 아니라 이번에 "새 후보를 찾았을 때
          // 딱 한 번"만 정리하면 된다.
          memmove(&rxpacket[0], &rxpacket[idx], rx_length - idx);
          rx_length -= idx;
          search_idx = 0;
          // idx==0으로 옮겨온 이 후보는, PKT_INSTRUCTION 등 뒤쪽 필드까지 안전하게 읽을 수
          // 있다는 보장이 이번 회차엔 없다(옮기기 전 idx가 limit 근처였다면 rx_length-idx가
          // 작을 수 있음). 그래서 즉시 검증하지 않고, 원본처럼 yield 후 다음 회차에서
          // rx_length >= wait_length 재확인을 거쳐 idx==0 분기로 다시 들어온다.
        }
        else
        {
          // idx == 0: 후보가 버퍼 맨 앞에 있다. 이 시점은 바깥 if에서 이미
          // rx_length >= wait_length(>=11)가 보장되어 있으므로 아래 필드 접근이 안전하다.
          if (rxpacket[PKT_RESERVED] != 0x00 ||
              DXL_MAKEWORD(rxpacket[PKT_LENGTH_L], rxpacket[PKT_LENGTH_H]) > RXPACKET_MAX_LEN ||
              rxpacket[PKT_INSTRUCTION] != 0x55)
          {
            // [증명] 후보가 FF FF FD(+stuffing 아님) 패턴은 만족하지만 패킷 내용 검증에 실패했다.
            // 앞 4바이트(0,1,2,3) 각각이 "새 헤더의 시작"이 될 수 있는지 위치별로 확정한다.
            //  - offset 0: 방금 패킷으로서 완전히 검증되어 실패했으므로 폐기 확정.
            //  - offset 1: 헤더가 되려면 offset(1+1)=2가 0xFF여야 하는데 이미 0xFD로 확인됨.
            //              이 값은 이미 버퍼에 존재하는 값이라 앞으로도 바뀌지 않으므로 영구 기각.
            //  - offset 2: 헤더가 되려면 offset 2 자신이 0xFF여야 하는데 0xFD다. 영구 기각.
            //  - offset 3: 우리가 아는 것은 "0xFD가 아니다"뿐이다. 0xFF일 수 있어 새 헤더의
            //              시작일 가능성이 남아있으므로 반드시 보존해야 한다.
            // 따라서 안전하게 버릴 수 있는 바이트 수는 정확히 3바이트(HEADER_LEN)이다.
            // 원본의 "1바이트만 버림"은 틀린 것은 아니지만(안전은 함) 과도하게 보수적이라
            // 같은 상황을 3배 더 많은 반복으로 통과했을 뿐이다.
            memmove(&rxpacket[0], &rxpacket[HEADER_LEN], rx_length - HEADER_LEN);
            rx_length -= HEADER_LEN;
            search_idx = 0;
            continue;   // 원본처럼 즉시 재시도 (yield 생략)
          }

          // 헤더 구조 + 내용 검증까지 모두 통과 = 진짜 헤더로 확정.
          uint16_t real_len = DXL_MAKEWORD(rxpacket[PKT_LENGTH_L], rxpacket[PKT_LENGTH_H])
                              + PKT_LENGTH_H + 1;
          if (wait_length != real_len)
          {
            wait_length = real_len;
            header_confirmed = true;
            continue;   // 원본처럼 즉시 재시도 (yield 생략). 다음 회차에서 rx_length>=wait_length 재검증.
          }
          header_confirmed = true;
          // [5번 항목] 원본에는 여기서 `if (rx_length < wait_length) {...}` 블록이 있었다.
          // 이 지점에 도달했다는 것은 바깥 if(rx_length>=wait_length)가 이미 참이고, 그 사이
          // rx_length/wait_length가 전혀 바뀌지 않았다는 뜻이므로 rx_length<wait_length는
          // 항상 거짓이다. 즉 도달 불가능한 코드였으므로 제거했다(동작 변화 없음).
          // continue하지 않고 아래로 흘러가서 바로 CRC 검사로 들어간다(원본과 동일 회차 내 처리).
        }
      }

      if (header_confirmed)
      {
        // 이 지점은 "이번 회차에 막 확정됨" 또는 "이전 회차에 이미 확정되어 데이터만 더 채움"
        // 두 경우 모두를 포함한다. 어느 쪽이든 바깥 if에서 rx_length>=wait_length가 보장된다.
        uint16_t crc = DXL_MAKEWORD(rxpacket[wait_length - 2], rxpacket[wait_length - 1]);
        result = (updateCRC(0, rxpacket, wait_length - 2) == crc) ? COMM_SUCCESS : COMM_RX_CORRUPT;
        break;
      }
    }
    else
    {
      // rx_length < wait_length: 아직 필요한 만큼 못 받음. 원본과 동일한 timeout 처리.
      if (port->isPacketTimeout() == true)
      {
        result = (rx_length == 0) ? COMM_RX_TIMEOUT : COMM_RX_CORRUPT;
        break;
      }
    }

#if defined(__linux__) || defined(__APPLE__)
    usleep(0);
#elif defined(_WIN32) || defined(_WIN64)
    Sleep(0);
#elif defined(ESP_PLATFORM)
    // taskYIELD()는 "같은 우선순위의 다른 태스크"에게만 양보한다. 이 태스크가 idle보다 높은 우선순위에서 계속 ready 상태라면 idle 태스크(및 watchdog feed)가 전혀 실행되지
    // 못해 Task Watchdog reset을 유발할 수 있다. 확실히 안전하려면 최소 1 tick의 실제 delay가 필요하다. 다만 이 delay는 (특히 half-duplex 응답을 기다리는 구간에서) 왕복
    // 타이밍에 영향을 줄 수 있으므로, tick rate를 충분히 높이거나(예: 1kHz), 응답을 이미 기다리는 도중이 아니라 유휴 구간에서만 이 폴링이 자주 발생하도록 상위 타임아웃 설계와 맞춰 사용하는 것을 권장한다.
    vTaskDelay(1);
#endif
  }

  port->is_using_ = false;

  if ((result == COMM_SUCCESS) && (false == skip_stuffing))
    removeStuffing(rxpacket);

  return result;
}






// packet_data(86) = TIME(2) + 12*[ACC(1) + POS(2) + MAX_TIME(2) + VEL(2)]
static int protocol::packet_parser(packet_t *packet,uint8_t byte)
{
    /*패킷을 읽던중 ff,ff가 들어오면 새 패킷으로 시작하지 않고
    기존에 읽던 패킷을 len을 참고해서 다 읽고,
    check_sum확인했을때 맞다면 중간에 읽은 ff,ff를
    기존 패킷의 데이터의 일부라고 보도록 하는게 합리적*/
    // 체크섬 실패시 버렸던 버퍼에서 헤더 유무파악하기

    /* 1. 첫 번째 FF 탐색 */
    if (packet->idx == 0)
    {
        if (byte == 0xFF)
        {
            packet->buffer[0] = byte;
            packet->idx = 1;
        }
        return false;
    }

    /* 2. 두 번째 FF 확인 */
    if (packet->idx == 1)
    {
        if (byte == 0xFF)
        {
            packet->buffer[1] = byte;
            packet->idx = 2;}
        else
        {packet->idx = 0;}
        return false;
    }

    if (packet->idx >= sizeof(packet->buffer))
    {
        packet->idx = 0;
        packet->len = 0;
        return false;
    }

    packet->buffer[packet->idx] = byte;
    packet->idx++;

    /* 4. LEN 확인 */
    if (packet->idx == 3)
    {
        packet->len = packet->buffer[2];

        if (packet->len < 5)
        {
            packet->idx = 0;
            packet->len = 0;
            return false;
        }

        if (packet->len > sizeof(packet->buffer))
        {
            packet->idx = 0;
            packet->len = 0;
            return false;
        }
    }

    /* 5. 패킷 완성 확인 */
    if (packet->idx >= 5 && packet->idx == packet->len)
    {
        uint16_t sum = 0;
        for (size_t i = 2; i < packet->idx - 1; i++)
        {sum += packet->buffer[i];} // without header(0xff,0xff)
        uint8_t checksum = (uint8_t)(sum & 0xFF);

        if (checksum == packet->buffer[packet->len - 1])
        {   packet->idx = 0;
            packet->len = 0;
            uint8_t inst = packet->buffer[3];

            switch (inst)
            {
                case INST_REGISTER_TRAJECTORY:
                    register_joint_trajectory(packet);
                    break;

                case INST_STOP:
                    stop_motor();
                    break;

                case INST_CLEAR_TRAJECTORY:
                    clear_trajectory();
                    break;

                default:
                    // 알 수 없는 INST
                    break;
            }
            return true;}
    }
    return false;
}
