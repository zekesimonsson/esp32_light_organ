#include "light_control.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#define LIGHT_CONTROL_QUEUE_LENGTH 10

static QueueHandle_t command_queue = NULL;
static SemaphoreHandle_t status_mutex = NULL;

static light_control_status_t current_status = {
    .mode = LIGHT_EFFECT_ALTERNATE,
    .brightness = 30,
    .beat_brightness = 45,
    .blackout = false
};

void light_control_init(void)
{
    command_queue = xQueueCreate(
        LIGHT_CONTROL_QUEUE_LENGTH,
        sizeof(light_control_command_t));

    status_mutex = xSemaphoreCreateMutex();

    configASSERT(command_queue != NULL);
    configASSERT(status_mutex != NULL);

    light_effects_set_mode(current_status.mode);
    light_effects_set_brightness(current_status.brightness);
    light_effects_set_beat_brightness(
        current_status.beat_brightness);
}

bool light_control_send(
    const light_control_command_t *command)
{
    if ((command_queue == NULL) || (command == NULL))
    {
        return false;
    }

    return xQueueSend(
        command_queue,
        command,
        0) == pdTRUE;
}

void light_control_process(void)
{
    light_control_command_t command;

    if (command_queue == NULL)
    {
        return;
    }

    while (xQueueReceive(
        command_queue,
        &command,
        0) == pdTRUE)
    {
        bool valid = true;

        xSemaphoreTake(status_mutex, portMAX_DELAY);

        switch (command.type)
        {
            case LIGHT_CONTROL_SET_MODE:
                if (command.value < LIGHT_EFFECT_COUNT)
                {
                    current_status.mode =
                        (light_effect_mode_t)command.value;

                    light_effects_set_mode(
                        current_status.mode);
                }
                else
                {
                    valid = false;
                }
                break;

            case LIGHT_CONTROL_SET_BRIGHTNESS:
                if (command.value <= 100)
                {
                    current_status.brightness = command.value;
                    light_effects_set_brightness(command.value);
                }
                else
                {
                    valid = false;
                }
                break;

            case LIGHT_CONTROL_SET_BEAT_BRIGHTNESS:
                if (command.value <= 100)
                {
                    current_status.beat_brightness = command.value;
                    light_effects_set_beat_brightness(command.value);
                }
                else
                {
                    valid = false;
                }
                break;

            case LIGHT_CONTROL_SET_BLACKOUT:
                if (command.value <= 1)
                {
                    current_status.blackout =
                        (command.value != 0);
                }
                else
                {
                    valid = false;
                }
                break;

            default:
                valid = false;
                break;
        }

        xSemaphoreGive(status_mutex);

        (void)valid;
    }
}

void light_control_get_status(
    light_control_status_t *status)
{
    if ((status == NULL) || (status_mutex == NULL))
    {
        return;
    }

    xSemaphoreTake(status_mutex, portMAX_DELAY);
    *status = current_status;
    xSemaphoreGive(status_mutex);
}