
// Motor Driver Subsystem (UART)
class MotorDriver {
public:
    esp_err_t init(uart_port_t port, int tx_pin, int rx_pin, uint32_t baud_rate);
    esp_err_t send_command(uint8_t id, uint8_t cmd, const uint8_t *data, size_t len);
    

private:
    uart_port_t uart_port = UART_NUM_MAX;
};