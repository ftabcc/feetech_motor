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

typedef struct
{
    size_t len;
    int inst;
    int err; // for emergency stop
    uint8_t data[PACKET_MAX_LEN];
    int crc;
} motor2esp_packet_t;
// HEAD(0xFF 0xFF) + ID(1) + LEN(1) + ERR(1) + DATA(N) + CHECK_SUM(1) = N+6(N>=0)


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