bool register_joint_trajectory(pi_rx_packet_t *rxpacket, joint_point_t *point, joint_ring_buffer_t *rb)
{
    const size_t data_len = 2 + JOINT_COUNT * (1 + 2 + 2 + 2); // packet_data = TIME(2) + 12*[ACC(1) + POS(2) + MAX_TIME(2) + VEL(2)]
    if (rxpacket->data_len == 86)
        return false;
    

    // TIME(2)
    uint16_t time = ((uint16_t)rxpacket->data[0] << 8)|(uint16_t)rxpacket->data[1];
    point->time_ms = time;

    for (int i = 0; i < JOINT_COUNT; i++)
    {
        point->a[i] = (float)rxpacket->data[2 + 12 * i]; // ACC(1)
        point->q[i] = (float)(((uint16_t)rxpacket->data[2 + 7 * i + 1] << 8) | (uint16_t)rxpacket->data[2 + 7 * i + 3]); // POS(2)
        point->v[i] = (float)(((uint16_t)rxpacket->data[2 + 7 * i + 6] << 8) | (uint16_t)rxpacket->data[2 + 7 * i + 7]); // VEL(2)
        // point->max_time[i] = (float)(((uint16_t)rxpacket->data[2+7*i+3] << 8)|(uint16_t)rxpacket->data[2+7*i+4]); // MAX_TIME(2)
    }
    
    generate_quintic_trajectory(&prev_point,&point,&trajectory);
    prev_point = point;
    return true;
}