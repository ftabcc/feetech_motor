#ifndef motor_comm
#define motor_comm

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define UART_PORT       UART_NUM_1
#define UART_TX_PIN     GPIO_NUM_17
#define UART_RX_PIN     GPIO_NUM_18
#define UART_BAUDRATE   115200

// typedef struct
// {
//     // size_t len;
//     // int inst;
//     // uint8_t data[PACKET_MAX_LEN];
//     // int crc;
// } esp2motor_packet_t;
// HEAD(0xFF 0xFF) + ID(1) + LEN(1) + INST(1) + DATA(N) + CHECK_SUM(1) = N+6(N>=0)


typedef struct {
    uint32_t t_ms;
    float q[JOINT_COUNT];
    float v[JOINT_COUNT];
    float a[JOINT_COUNT];
    //uint16_t max_time[12];
} state_point_t;// packet_data(86) = TIME(2) + 12*[ACC(1) + POS(2) + MAX_TIME(2) + VEL(2)]


typedef struct
{
    int id;
    size_t len;
    int err; // for emergency stop
    uint8_t data[PACKET_MAX_LEN];
    int crc;
} motor2esp_packet_t;
// HEAD(0xFF 0xFF) + ID(1) + LEN(1) + ERR(1) + DATA(N) + CHECK_SUM(1) = N+6(N>=0)


enum class trajectory_err_t
{
    SUCCESS = 0,
    INVALID_LENGTH = 1,
    INVALID_DURATION = 2,
    BUFFER_FULL = 3
};

class motor_comm
{
public:
    static void init();
    // esp2motor_packet_t txpacket;
    uint8_t txpacket[100];
    motor2esp_packet_t rxpacket;
    
private:
    static void rx_task(int itf,cdcacm_event_t *event);
    static int rx_packet(int itf);
    static void tx_packet(int itf,cdcacm_event_t *event);
private:
    waypoint_t waypoint;
    static QueueHandle_t uart_queue;
};

#endif