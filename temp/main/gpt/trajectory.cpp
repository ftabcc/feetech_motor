#include "trajectory.h"

bool register_trajectory(pi_rx_packet_t *rxpacket)
{
    if (rxpacket->len == 2 + JOINT_COUNT * (1 + 2 + 2 + 2)) // packet_data = TIME(2) + 12*[ACC(1) + POS(2) + MAX_TIME(2) + VEL(2)]
        return false;
    
    // TIME(2)
    p1->t_ms = ((uint16_t)rxpacket->data[0] << 8)|(uint16_t)rxpacket->data[1];

    for (int i = 0; i < JOINT_COUNT; i++)
    {
        p1->a[i] = (float)rxpacket->data[2 + 12 * i]; // ACC(1)
        p1->q[i] = (float)(((uint16_t)rxpacket->data[2 + 7 * i + 1] << 8) | (uint16_t)rxpacket->data[2 + 7 * i + 3]); // POS(2)
        p1->v[i] = (float)(((uint16_t)rxpacket->data[2 + 7 * i + 6] << 8) | (uint16_t)rxpacket->data[2 + 7 * i + 7]); // VEL(2)
        // point->max_time[i] = (float)(((uint16_t)rxpacket->data[2+7*i+3] << 8)|(uint16_t)rxpacket->data[2+7*i+4]); // MAX_TIME(2)
    }

    uint32_t duration = p1.t_ms - p0->t_ms;
    if (duration % CONTROL_PERIOD_MS != 0) || (duration < 0)
        return false;

    size_t count = duration / CONTROL_PERIOD_MS;
    if (count > TRAJECTORY_MAX_POINTS)
        return false;

    // without start point
    for (size_t i = 1; i < count + 1; i++) {
        uint32_t t_ms = i * CONTROL_PERIOD_MS;
        quintic_hermite(t_ms,&trajectory->points[trajectory->write_idx]);
        trajectory->write_idx = (trajectory->write_idx_idx + 1) % TRAJECTORY_BUFFER_SIZE;
        trajectory->count += count;
    }

    return true;
}

static void quintic_hermite(uint32_t t_ms,joint_point_t *point)
{
    float T = (float)(p1->time_ms - p0->time_ms) / 1000.0f;
    float t = (float)t_ms / 1000.0f;
    float s = t / T;

    float s2 = s * s;
    float s3 = s2 * s;
    float s4 = s3 * s;
    float s5 = s4 * s;

    float h0 = 1 - 10*s3 + 15*s4 - 6*s5;
    float h1 = 10*s3 - 15*s4 + 6*s5;
    float h2 = s - 6*s3 + 8*s4 - 3*s5;
    float h3 = -4*s3 + 7*s4 - 3*s5;
    float h4 = 0.5f*s2 - 1.5f*s3 + 1.5f*s4 - 0.5f*s5;
    float h5 = 0.5f*s3 - s4 + 0.5f*s5;

    float dh0 = -30*s2 + 60*s3 - 30*s4;
    float dh1 = 30*s2 - 60*s3 + 30*s4;
    float dh2 = 1 - 18*s2 + 32*s3 - 15*s4;
    float dh3 = -12*s2 + 28*s3 - 15*s4;
    float dh4 = s - 4.5f*s2 + 6*s3 - 2.5f*s4;
    float dh5 = 1.5f*s2 - 4*s3 + 2.5f*s4;

    float ddh0 = -60*s + 180*s2 - 120*s3;
    float ddh1 = 60*s - 180*s2 + 120*s3;
    float ddh2 = -36*s + 96*s2 - 60*s3;
    float ddh3 = -24*s + 84*s2 - 60*s3;
    float ddh4 = 1 - 9*s + 18*s2 - 10*s3;
    float ddh5 = 3*s - 12*s2 + 10*s3;

    point->time_ms = p0->time_ms + t_ms;

    for (int i = 0; i < JOINT_COUNT; i++) {
        point->q[i] = h0*p0->q[i] + h1*p1->q[i] + h2*T*p0->v[i] + h3*T*p1->v[i] + h4*T*T*p0->a[i] + h5*T*T*p1->a[i];
        point->v[i] = (dh0*p0->q[i] + dh1*p1->q[i] + dh2*T*p0->v[i] + dh3*T*p1->v[i] + dh4*T*T*p0->a[i] + dh5*T*T*p1->a[i]) / T;
        point->a[i] = (ddh0*p0->q[i] + ddh1*p1->q[i] + ddh2*T*p0->v[i] + ddh3*T*p1->v[i] + ddh4*T*T*p0->a[i] + ddh5*T*T*p1->a[i]) / (T*T);
    }
}
