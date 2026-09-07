#include "driver/uart.h"

#define UART_PORT       UART_NUM_1
#define UART_TX_PIN     GPIO_NUM_17
#define UART_RX_PIN     GPIO_NUM_18
#define UART_BAUDRATE   115200

void uart_init()
{
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config)); // 설정 
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT,UART_TX_PIN,UART_RX_PIN,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE)); // 핀 지정
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT,1024,1024,0,NULL,0)); // 설치

    // read task
    xTaskCreate(uart_rx_task,"uart_rx_task",4096,NULL,10,NULL);
    // write task

}


void motor_write_task(void *arg)
{
    trajectory_t *trajectory = (trajectory_t *)arg;

    while (true) {
        if (trajectory->read_idx == trajectory->write_idx) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        joint_point_t *point = &trajectory->points[trajectory->read_idx];

        uint32_t now_ms = esp_timer_get_time() / 1000;
        uint32_t target_ms = point->time_ms;

        if (target_ms > now_ms) {
            vTaskDelay(pdMS_TO_TICKS(target_ms - now_ms));
        }

        motor_control.set_point(point);

        uart_write_bytes(UART_PORT,packet,sizeof(packet));

        trajectory->read_idx = (trajectory->read_idx + 1) % TRAJECTORY_BUFFER_SIZE;
        trajectory->count += count;

    }
}

void motor_read_task(void *arg)
{
    int len = uart_read_bytes(UART_PORT,buffer,sizeof(buffer),pdMS_TO_TICKS(100));
}

