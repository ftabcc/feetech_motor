#include "protocol.h"

// packet_data(86) = TIME(2) + 12*[ACC(1) + POS(2) + MAX_TIME(2) + VEL(2)]

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

      // find packet header 만약 헤더검색 실패시 이미 봤던부분 skip할 수 있도록 업데이트하기.
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
          // remove the first byte in the packet 헤더검색했으나 진짜가 아니라면 헤더라고 판단한 바이트 수(4)만큼 건너뛰게 하기.
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
        // remove unnecessary packets
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
