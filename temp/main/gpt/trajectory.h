#ifndef TRAJECTORY_H
#define TRAJECTORY_H

#include <stdint.h>
#include <stddef.h>

#define JOINT_COUNT 12
#define CONTROL_PERIOD_MS 20
#define TRAJECTORY_MAX_POINTS 100

typedef struct {
    uint32_t t_ms;
    float q[JOINT_COUNT];
    float v[JOINT_COUNT];
    float a[JOINT_COUNT];
    //uint16_t max_time[12];
} waypoint_t;// packet_data(86) = TIME(2) + 12*[ACC(1) + POS(2) + MAX_TIME(2) + VEL(2)]

typedef struct {
    waypoint_t points[TRAJECTORY_MAX_POINTS];
    size_t write_idx;   // 다음에 넣을 위치
    size_t read_idx;  // 다음에 꺼낼 위치
    size_t count;
} trajectory_t;


class trajectory
{
public:
    trajectory_t trajectory;
    waypoint_t p0;
    waypoint_t p1;

private:
    bool register_trajectory(pi_rx_packet_t *rxpacket)
    static void quintic_hermite(uint32_t t_ms,joint_point_t *point)
};

#endif