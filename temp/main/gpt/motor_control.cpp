
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
        trajectory->read_idx = (trajectory->read_idx + 1) % TRAJECTORY_BUFFER_SIZE;
        trajectory->count += count;

    }
}

void motor_read_task(void *arg)
{

}