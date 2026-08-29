
#define INST_REGISTER_TRAJECTORY  0x01
#define INST_STOP                 0x02
#define INST_CLEAR_TRAJECTORY     0x03

bool register_joint_trajectory(const packet_t *packet,joint_point_t *point, joint_ring_buffer_t *rb)
{
    if (packet == NULL || point == NULL)
        return false;    

    const size_t data_len = 2 + JOINT_COUNT * (1 + 2 + 2 + 2); // packet_data = TIME(2) + 12*[ACC(1) + POS(2) + MAX_TIME(2) + VEL(2)]
    if (packet->len < 4 + data_len + 1)
        return false;
    
    size_t idx = 4;

    // TIME(2)
    uint16_t time = ((uint16_t)packet->buffer[idx] << 8)|(uint16_t)packet->buffer[idx + 1];
    point->time_ms = time;
    idx += 2;

    for (int i = 0; i < JOINT_COUNT; i++)
    {
        // ACC(1)
        uint8_t acc = packet->buffer[idx];
        idx += 1;
        // POS(2)
        uint16_t pos = ((uint16_t)packet->buffer[idx] << 8)|(uint16_t)packet->buffer[idx + 1];
        idx += 2;
        // MAX_TIME(2)
        uint16_t max_time = ((uint16_t)packet->buffer[idx] << 8)|(uint16_t)packet->buffer[idx + 1];
        idx += 2;
        // VEL(2)
        uint16_t vel = ((uint16_t)packet->buffer[idx] << 8)|(uint16_t)packet->buffer[idx + 1];
        idx += 2;
        // joint_point_t에 저장
        point->a[i] = (float)acc;
        point->q[i] = (float)pos;
        // point->max_time[i] = max_time;
        point->v[i] = (float)vel;
    }
    generate_quintic_trajectory(&prev_point,&point,&trajectory);
    prev_point = point;
    return true;
}