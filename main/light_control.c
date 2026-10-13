#include "light_control.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define LIGHT_CONTROL_QUEUE_LENGTH 10

static QueueHandle_t command_queue = NULL;

void light_control_init(void)
{
    command_queue = xQueueCreate(
        LIGHT_CONTROL_QUEUE_LENGTH,
        sizeof(light_control_command_t));

    configASSERT(command_queue != NULL);
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
        switch (command.type)
        {
            case LIGHT_CONTROL_SET_MODE:
                if (command.value < LIGHT_EFFECT_COUNT)
                {
                    light_effects_set_mode(
                        (light_effect_mode_t)command.value);
                }
                break;

            case LIGHT_CONTROL_SET_BRIGHTNESS:
                light_effects_set_brightness(
                    command.value);
                break;

            case LIGHT_CONTROL_SET_BEAT_BRIGHTNESS:
                light_effects_set_beat_brightness(
                    command.value);
                break;

            default:
                break;
        }
    }
}