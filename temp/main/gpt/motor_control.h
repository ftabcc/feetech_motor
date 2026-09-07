#ifndef motor_comm
#define motor_comm

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class motor_comm
{
public:
    static void init();
private:
    static void rx_task(int itf,cdcacm_event_t *event);
    static int rx_packet(int itf);
    static void tx_task(int itf,cdcacm_event_t *event);
    static int tx_packet(int itf);

private:
};


#endif