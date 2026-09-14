bool register_trajectory(pi_rx_packet_t *rxpacket)
{
    if (rxpacket->len == 2 + JOINT_COUNT * (1 + 2 + 2 + 2)) // packet_data = TIME(2) + 12*[ACC(1) + POS(2) + MAX_TIME(2) + VEL(2)]
        return false;
    
    // TIME(2)
    uint16_t time = ((uint16_t)rxpacket->data[0] << 8)|(uint16_t)rxpacket->data[1];
    waypoint->time_ms = time;

    for (int i = 0; i < JOINT_COUNT; i++)
    {
        waypoint->a[i] = (float)rxpacket->data[2 + 12 * i]; // ACC(1)
        waypoint->q[i] = (float)(((uint16_t)rxpacket->data[2 + 7 * i + 1] << 8) | (uint16_t)rxpacket->data[2 + 7 * i + 3]); // POS(2)
        waypoint->v[i] = (float)(((uint16_t)rxpacket->data[2 + 7 * i + 6] << 8) | (uint16_t)rxpacket->data[2 + 7 * i + 7]); // VEL(2)
        // point->max_time[i] = (float)(((uint16_t)rxpacket->data[2+7*i+3] << 8)|(uint16_t)rxpacket->data[2+7*i+4]); // MAX_TIME(2)
    }
    
    generate_quintic_trajectory(&prev_point,&waypoint,&trajectory);
    prev_point = waypoint;
    return true;
}
{
    if (!p0 || !p1 || !trajectory)
        return false;

    if (p1->time_ms <= p0->time_ms)
        return false;

    uint32_t duration = p1->time_ms - p0->time_ms;

    if (duration % CONTROL_PERIOD_MS != 0)
        return false;

    size_t count = duration / CONTROL_PERIOD_MS;

    if (count > TRAJECTORY_MAX_POINTS)
        return false;
    
    // without start point
    for (size_t i = 1; i < count + 1; i++) {
        uint32_t t_ms = i * CONTROL_PERIOD_MS;
        quintic_hermite(p0, p1, t_ms,&trajectory->points[trajectory->write_idx]);
        trajectory->write_idx = (trajectory->write_idx_idx + 1) % TRAJECTORY_BUFFER_SIZE;
        trajectory->count += count;
    }

    return true;
}