#include "protocol.h"

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
