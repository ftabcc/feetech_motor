#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class ESPMain
{
private:
    TaskHandle_t rx_task_handle = nullptr;
    TaskHandle_t control_task_handle = nullptr;
    TaskHandle_t observation_task_handle = nullptr;

    bool running = true;

    // FreeRTOS task entry
    static void rx_task_entry(void* arg)
    {
    ESPMain* self = static_cast<ESPMain*>(arg);
    self->get_trajectory();
    vTaskDelete(nullptr);}

    static void control_task_entry(void* arg)
    {
        ESPMain* self = static_cast<ESPMain*>(arg);
        self->set_trajectory();
        vTaskDelete(nullptr);
    }

    static void observation_task_entry(void* arg)
    {
        ESPMain* self = static_cast<ESPMain*>(arg);
        self->send_observation();
        vTaskDelete(nullptr);
    }


    // 실제 작업
    void get_trajectory()
    {
        while (running)
        {
            // trajectory 수신
        }
    }

    void set_trajectory()
    {
        while (running)
        {
            // trajectory 실행
        }
    }

    void send_observation()
    {
        while (running)
        {
            // observation 전송
        }
    }


public:
    ESPMain()
    {
        // 3개의 Task 생성
        xTaskCreate(
            rx_task_entry,
            "rx_task",
            4096,
            this,
            5,
            &rx_task_handle
        );

        xTaskCreate(
            control_task_entry,
            "control_task",
            4096,
            this,
            6,
            &control_task_handle
        );

        xTaskCreate(
            observation_task_entry,
            "observation_task",
            4096,
            this,
            4,
            &observation_task_handle
        );
    }


    void stop()
    {
        running = false;

        // 필요한 경우 Task가 대기 상태에서 빠져나오도록
        // notification 등을 사용할 수 있음.
    }
};


extern "C" void app_main()
{
    static ESPMain esp;

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}